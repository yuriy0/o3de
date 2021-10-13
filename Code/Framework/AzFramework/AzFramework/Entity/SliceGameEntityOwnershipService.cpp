/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/SliceGameEntityOwnershipService.h>

namespace AzFramework
{
    SliceGameEntityOwnershipService::SliceGameEntityOwnershipService(const EntityContextId& entityContextId,
        AZ::SerializeContext* serializeContext)
        : SliceEntityOwnershipService(entityContextId, serializeContext)
    {
        SliceGameEntityOwnershipServiceRequestBus::Handler::BusConnect();
    }

    SliceGameEntityOwnershipService::~SliceGameEntityOwnershipService()
    {
        SliceGameEntityOwnershipServiceRequestBus::Handler::BusDisconnect();
    }

    void SliceGameEntityOwnershipService::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SliceGameEntityOwnershipServiceRequestBus>("SliceGameEntityOwnershipServiceRequestBus")
                ->Attribute(AZ::Script::Attributes::Module, "entity")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Event("DestroyDynamicSliceByEntity", &SliceGameEntityOwnershipServiceRequestBus::Events::DestroyDynamicSliceByEntity)
                ;
        }
    }

    void SliceGameEntityOwnershipService::CreateRootSlice()
    {
        AZ_Assert(GetRootAsset() && GetRootAsset().Get(), "Root slice asset has not been created yet.");
        AZ::SliceAsset* rootSliceAsset = GetRootAsset().Get();
        if (rootSliceAsset->GetEntity() != nullptr)
        {
            // Clearing dynamic slice destruction queue now since all slices in it 
            // are being deleted during the destruction phase.
            // We don't want this list holding onto deleted slices!
            m_dynamicSlicesToDestroy.clear();
        }
        SliceEntityOwnershipService::CreateRootSlice(rootSliceAsset);

        // We want all dynamic slices spawned in the game entity ownership service to be
        // instantiated, which depends on the root slice itself being instantiated.
        rootSliceAsset->GetComponent()->Instantiate();
    }

    void SliceGameEntityOwnershipService::Reset()
    {
        SliceEntityOwnershipService::Reset();
        SliceInstantiationResultBus::MultiHandler::BusDisconnect();
        m_instantiatingDynamicSlices.clear();
    }

    //=========================================================================
    // SliceGameEntityOwnershipServiceRequestBus::InstantiateDynamicSlice
    //=========================================================================
    SliceInstantiationTicket SliceGameEntityOwnershipService::InstantiateDynamicSlice(
        const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, const AZ::Transform& worldTransform,
        const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper)
    {
        if (sliceAsset.GetId().IsValid())
        {
            const SliceInstantiationTicket ticket = InstantiateSlice(sliceAsset, customIdMapper);
            if (ticket.IsValid())
            {
                InstantiatingDynamicSliceInfo& info = m_instantiatingDynamicSlices[ticket];
                info.m_asset = sliceAsset;
                info.m_transform = worldTransform;

                SliceInstantiationResultBus::MultiHandler::BusConnect(ticket);

                return ticket;
            }
        }

        return SliceInstantiationTicket();
    }

    //=========================================================================
    // SliceGameEntityOwnershipServiceRequestBus::InstantiateDynamicSliceBlocking
    //=========================================================================
    AZ::SliceComponent::SliceInstanceAddress SliceGameEntityOwnershipService::InstantiateDynamicSliceBlocking(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        const AZ::Transform& worldTransform, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper,
        const AZStd::function<void(const AZ::SliceComponent::SliceInstanceAddress&)>& preInstantiate)
    {
        if (asset.GetId().IsValid())
        {
            const_cast<AZ::Data::Asset<AZ::Data::AssetData>&>(asset).QueueLoad();
            while (asset.IsLoading()) {}
            if (asset.IsReady())
            {
                AZ::SliceComponent::SliceInstanceAddress instance = GetRootAsset().Get()->GetComponent()->AddSlice(asset, customIdMapper);

                if (instance.second) {
                    AZ_Assert(instance.second->GetInstantiated(), "Failed to instantiate root slice!");

                    if (instance.second->GetInstantiated() &&
                        m_validateEntitiesCallback(instance.second->GetInstantiated()->m_entities))
                    {
                        // Call OnSlicePreInstantiate which sets world TM and handles world entity ID reference remapping
                        OnSlicePreInstantiate(InstantiatingDynamicSliceInfo{ asset, worldTransform }, instance);

                        // Handle user-defined preInstantiate callback
                        if (preInstantiate) preInstantiate(instance);

                        // Finally add the entities to the entity context
                        HandleEntitiesAdded(instance.second->GetInstantiated()->m_entities);

                        return instance;
                    }
                    else
                    {
                        // The prefab has already been added to the root slice. But we are disallowing the
                        // instantiation. So we need to remove it
                        GetRootAsset().Get()->GetComponent()->RemoveSlice(asset);
                    }
                }
            }
        }

        return AZ::SliceComponent::SliceInstanceAddress(nullptr, nullptr);
    }

    //=========================================================================
    // SliceGameEntityOwnershipServiceRequestBus::CancelDynamicSliceInstantiation
    //=========================================================================
    void SliceGameEntityOwnershipService::CancelDynamicSliceInstantiation(const SliceInstantiationTicket& ticket)
    {
        // Cleanup of m_instantiatingDynamicSlices will be handled by OnSliceInstantiationFailed()
        CancelSliceInstantiation(ticket);
    }

    //=========================================================================
    // SliceGameEntityOwnershipServiceRequestBus::DestroyDynamicSliceByEntity
    //=========================================================================
    bool SliceGameEntityOwnershipService::DestroyDynamicSliceByEntity(const AZ::EntityId& id)
    {
        AZ::SliceComponent* rootSlice = GetRootSlice();
        if (rootSlice)
        {
            const auto address = rootSlice->FindSlice(id);
            if (address.GetInstance())
            {
                auto sliceInstance = address.GetInstance();
                const auto instantiatedSliceEntities = sliceInstance->GetInstantiated();
                if (instantiatedSliceEntities)
                {
                    for (AZ::Entity* currentEntity : instantiatedSliceEntities->m_entities)
                    {
                        if (currentEntity)
                        {
                            if (currentEntity->GetState() == AZ::Entity::State::Active)
                            {
                                currentEntity->Deactivate();
                            }
                            MarkEntityForNoActivation(currentEntity->GetId());
                        }
                    }
                }

                // Queue Slice deletion until next tick. This prevents deleting a dynamic slice from an active entity within that slice.
                m_dynamicSlicesToDestroy.insert(address);

                AZStd::function<void()> deleteDynamicSlices = [this]()
                {
                    this->FlushDynamicSliceDeletionList();
                };

                AZ::TickBus::QueueFunction(deleteDynamicSlices);
                return true;
            }
        }

        return false;
    }

    void SliceGameEntityOwnershipService::FlushDynamicSliceDeletionList()
    {
        for (const auto& sliceAddress : m_dynamicSlicesToDestroy)
        {
            GetRootSlice()->RemoveSliceInstance(sliceAddress);
        }

        m_dynamicSlicesToDestroy.clear();
    }

    void SliceGameEntityOwnershipService::MarkEntityForNoActivation(AZ::EntityId entityId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

        AZ_Error("SliceGameEntityOwnershipService", entity,
            "Failed to locate entity with id %s. It is either not yet Initialized, or the Id is invalid.",
            entityId.ToString().c_str());

        if (entity)
        {
            entity->SetRuntimeActiveByDefault(false);
        }
    }

    //=========================================================================
    // SliceInstantiationResultBus::OnSlicePreInstantiate
    //=========================================================================
    void SliceGameEntityOwnershipService::OnSlicePreInstantiate(const InstantiatingDynamicSliceInfo& instantiating, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;

        // If the entity ownership service was loaded from a stream and Ids were remapped, fix up entity Ids in that slice that
        // point to entities in the stream (i.e. level entities).
        if (!GetLoadedEntityIdMap().empty())
        {
            AZ::EntityUtils::SerializableEntityContainer instanceEntities;
            instanceEntities.m_entities = entities;
            AZ::IdUtils::Remapper<AZ::EntityId>::RemapIds(&instanceEntities,
                [this](const AZ::EntityId& originalId, bool isEntityId, const AZStd::function<AZ::EntityId()>&) -> AZ::EntityId
                {
                    if (!isEntityId)
                    {
                        const AZ::SliceComponent::EntityIdToEntityIdMap& loadedEntityIdMap =
                            GetLoadedEntityIdMap();
                        auto iter = loadedEntityIdMap.find(originalId);
                        if (iter != loadedEntityIdMap.end())
                        {
                            return iter->second;
                        }
                    }
                    return originalId;

                }, GetSerializeContext(), false);
        }

        // Set initial transform for slice root entity based on the requested root transform for the instance.
        for (AZ::Entity* entity : entities)
        {
            auto* transformComponent = entity->FindComponent<AzFramework::TransformComponent>();
            if (transformComponent)
            {
                // Non-root entities will be positioned relative to their parents.
                if (!transformComponent->GetParentId().IsValid())
                {
                    // Note: Root slice entity always has translation at origin, so this maintains scale & rotation.
                    transformComponent->SetWorldTM(instantiating.m_transform * transformComponent->GetWorldTM());
                }
            }
        }
    }

    //=========================================================================
    // SliceInstantiationResultBus::OnSlicePreInstantiate
    //=========================================================================
    void SliceGameEntityOwnershipService::OnSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        const SliceInstantiationTicket ticket = *SliceInstantiationResultBus::GetCurrentBusId();

        auto instantiatingIter = m_instantiatingDynamicSlices.find(ticket);
        if (instantiatingIter != m_instantiatingDynamicSlices.end())
        {
            InstantiatingDynamicSliceInfo& instantiating = instantiatingIter->second;
            OnSlicePreInstantiate(instantiating, sliceAddress);
        }
    }

    //=========================================================================
    // SliceInstantiationResultBus::OnSliceInstantiated
    //=========================================================================
    void SliceGameEntityOwnershipService::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& instance)
    {
        const SliceInstantiationTicket ticket = *SliceInstantiationResultBus::GetCurrentBusId();

        if (m_instantiatingDynamicSlices.erase(ticket) > 0)
        {
            SliceGameEntityOwnershipServiceNotificationBus::Broadcast(
                &SliceGameEntityOwnershipServiceNotificationBus::Events::OnSliceInstantiated, sliceAssetId, instance, ticket);
        }

        SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);
    }

    //=========================================================================
    // SliceInstantiationResultBus::OnSliceInstantiationFailed
    //=========================================================================
    void SliceGameEntityOwnershipService::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
    {
        const SliceInstantiationTicket ticket = *SliceInstantiationResultBus::GetCurrentBusId();

        if (m_instantiatingDynamicSlices.erase(ticket) > 0)
        {
            SliceGameEntityOwnershipServiceNotificationBus::Broadcast(
                &SliceGameEntityOwnershipServiceNotificationBus::Events::OnSliceInstantiationFailed, sliceAssetId, ticket);
        }

        SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);
    }
}
