/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// APC BEGIN: still needed?
// Need to disable these warning for any_numeric_cast to work
// Also, it seems that including any_numeric_cast causes the 2nd warning to pop up everywhere in the file (?)
//#pragma warning(push)
//#pragma warning(disable : 4800) // forcing value to bool 'true' or 'false' (performance warning)
//#pragma warning(disable : 4267) // conversion from 'size_t' to 'AZ::u32', possible loss of data

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <MCore/Source/AttributeString.h>
#include <MCore/Source/AzCoreConversions.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/MotionSet.h>

#include <EMotionFX/Source/AnimGraphPlaySpeedModifier.h>

#include <Integration/Components/AnimGraphComponent.h>
#include <EMotionFX/Source/Parameter/StringParameter.h>
#include <EMotionFX/Source/AnimGraphNode.h>

namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        class AnimGraphComponentNotificationBehaviorHandler
            : public AnimGraphComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(AnimGraphComponentNotificationBehaviorHandler, "{ECFDB974-8C47-467C-8476-258BF57B3395}", AZ::SystemAllocator,
                OnAnimGraphInstanceCreated, OnAnimGraphInstanceDestroyed, OnAnimGraphFloatParameterChanged, OnAnimGraphBoolParameterChanged,
                OnAnimGraphStringParameterChanged, OnAnimGraphVector2ParameterChanged, OnAnimGraphVector3ParameterChanged, OnAnimGraphRotationParameterChanged, OnAnimGraphParameterChanged);

            void OnAnimGraphInstanceCreated(EMotionFX::AnimGraphInstance* animGraphInstance) override
            {
                Call(FN_OnAnimGraphInstanceCreated, animGraphInstance);
            }

            void OnAnimGraphInstanceDestroyed(EMotionFX::AnimGraphInstance* animGraphInstance) override
            {
                Call(FN_OnAnimGraphInstanceDestroyed, animGraphInstance);
            }

            void OnAnimGraphFloatParameterChanged(EMotionFX::AnimGraphInstance* animGraphInstance, size_t parameterIndex, float beforeValue, float afterValue) override
            {
                Call(FN_OnAnimGraphFloatParameterChanged, animGraphInstance, parameterIndex, beforeValue, afterValue);
            }

            void OnAnimGraphBoolParameterChanged(EMotionFX::AnimGraphInstance* animGraphInstance, size_t parameterIndex, bool beforeValue, bool afterValue) override
            {
                Call(FN_OnAnimGraphBoolParameterChanged, animGraphInstance, parameterIndex, beforeValue, afterValue);
            }

            void OnAnimGraphStringParameterChanged(EMotionFX::AnimGraphInstance* animGraphInstance, size_t parameterIndex, const char* beforeValue, const char* afterValue) override
            {
                Call(FN_OnAnimGraphStringParameterChanged, animGraphInstance, parameterIndex, beforeValue, afterValue);
            }

            void OnAnimGraphVector2ParameterChanged(EMotionFX::AnimGraphInstance* animGraphInstance, size_t parameterIndex, const AZ::Vector2& beforeValue, const AZ::Vector2& afterValue) override
            {
                Call(FN_OnAnimGraphVector2ParameterChanged, animGraphInstance, parameterIndex, beforeValue, afterValue);
            }

            void OnAnimGraphVector3ParameterChanged(EMotionFX::AnimGraphInstance* animGraphInstance, size_t parameterIndex, const AZ::Vector3& beforeValue, const AZ::Vector3& afterValue) override
            {
                Call(FN_OnAnimGraphVector3ParameterChanged, animGraphInstance, parameterIndex, beforeValue, afterValue);
            }

            void OnAnimGraphRotationParameterChanged(EMotionFX::AnimGraphInstance* animGraphInstance, size_t parameterIndex, const AZ::Quaternion& beforeValue, const AZ::Quaternion& afterValue) override
            {
                Call(FN_OnAnimGraphVector3ParameterChanged, animGraphInstance, parameterIndex, beforeValue, afterValue);
            }

            void OnAnimGraphParameterChanged(EMotionFX::AnimGraphInstance* animGraphInstance, size_t parameterIndex, AZStd::any beforeValue, AZStd::any afterValue) override
			{
                Call(FN_OnAnimGraphParameterChanged, animGraphInstance, parameterIndex, beforeValue, afterValue);
            };
        };

        //////////////////////////////////////////////////////////////////////////
        AnimGraphComponent::ParameterDefaults::~ParameterDefaults()
        {
            Reset();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::ParameterDefaults::Reset()
        {
            for (AZ::ScriptProperty* p : m_parameters)
            {
                delete p;
            }
            m_parameters.clear();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::ParameterDefaults::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ParameterDefaults>()
                    ->Version(1)
                    ->Field("Parameters", &ParameterDefaults::m_parameters)
                ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::Configuration::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<Configuration>()
                    ->Version(1)
                    ->Field("AnimGraphAsset", &Configuration::m_animGraphAsset)
                    ->Field("MotionSetAsset", &Configuration::m_motionSetAsset)
                    ->Field("ActiveMotionSetName", &Configuration::m_activeMotionSetName)
                    ->Field("ParameterDefaults", &Configuration::m_parameterDefaults)
                    ->Field("DebugVisualize", &Configuration::m_visualize)
                ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::Reflect(AZ::ReflectContext* context)
        {
            ParameterDefaults::Reflect(context);
            Configuration::Reflect(context);

            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<AnimGraphComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Configuration", &AnimGraphComponent::m_configuration)
                ;
            }

            auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Constant("InvalidParameterIndex", BehaviorConstant(InvalidIndex));

                behaviorContext->Class<AnimGraphPlaySpeedModifier>()
                    ->Method("Emplace", &AnimGraphPlaySpeedModifier::Emplace)
                    ->Method("Remove", &AnimGraphPlaySpeedModifier::Remove)
                    ->Method("Stop", &AnimGraphPlaySpeedModifier::Stop)
                    ->Method("GetRemainingDuration", &AnimGraphPlaySpeedModifier::GetRemainingDuration)
                    ->Method("GetSpeedMultiplier", &AnimGraphPlaySpeedModifier::GetSpeedMultiplier)
                    ;

                behaviorContext->EBus<AnimGraphComponentRequestBus>("AnimGraphComponentRequestBus")
                    // General API
                    ->Event("FindParameterIndex", &AnimGraphComponentRequestBus::Events::FindParameterIndex)
                    // Playspeed modifier
                    ->Event("AddPlayspeedModifier", &AnimGraphComponentRequestBus::Events::AddPlayspeedModifier)
                    ->Event("FindParameterName", &AnimGraphComponentRequestBus::Events::FindParameterName)
                    
                // Parameter setters
                    ->Event("SetParameter", &AnimGraphComponentRequestBus::Events::SetParameter)
                    ->Event("SetNamedParameter", &AnimGraphComponentRequestBus::Events::SetNamedParameter)

                    // Setters
                    ->Event("SetParameterFloat", &AnimGraphComponentRequestBus::Events::SetParameterFloat)
                    ->Event("SetParameterBool", &AnimGraphComponentRequestBus::Events::SetParameterBool)
                    ->Event("SetParameterString", &AnimGraphComponentRequestBus::Events::SetParameterString)
                    ->Event("SetParameterVector2", &AnimGraphComponentRequestBus::Events::SetParameterVector2)
                    ->Event("SetParameterVector3", &AnimGraphComponentRequestBus::Events::SetParameterVector3)
                    ->Event("SetParameterRotationEuler", &AnimGraphComponentRequestBus::Events::SetParameterRotationEuler)
                    ->Event("SetParameterRotation", &AnimGraphComponentRequestBus::Events::SetNamedParameterRotation)
                    ->Event("SetNamedParameterFloat", &AnimGraphComponentRequestBus::Events::SetNamedParameterFloat)
                    ->Event("SetNamedParameterBool", &AnimGraphComponentRequestBus::Events::SetNamedParameterBool)
                    ->Event("SetNamedParameterString", &AnimGraphComponentRequestBus::Events::SetNamedParameterString)
                    ->Event("SetNamedParameterVector2", &AnimGraphComponentRequestBus::Events::SetNamedParameterVector2)
                    ->Event("SetNamedParameterVector3", &AnimGraphComponentRequestBus::Events::SetNamedParameterVector3)
                    ->Event("SetNamedParameterRotationEuler", &AnimGraphComponentRequestBus::Events::SetNamedParameterRotationEuler)
                    ->Event("SetNamedParameterRotation", &AnimGraphComponentRequestBus::Events::SetNamedParameterRotation)
                // Parameter getters
                    ->Event("GetParameter", &AnimGraphComponentRequestBus::Events::GetParameter)
                    ->Event("GetNamedParameter", &AnimGraphComponentRequestBus::Events::GetNamedParameter)

                    // Parameter getters
                    ->Event("SetVisualizeEnabled", &AnimGraphComponentRequestBus::Events::SetVisualizeEnabled)
                    // Getters
                    ->Event("GetParameterFloat", &AnimGraphComponentRequestBus::Events::GetParameterFloat)
                    ->Event("GetParameterBool", &AnimGraphComponentRequestBus::Events::GetParameterBool)
                    ->Event("GetParameterString", &AnimGraphComponentRequestBus::Events::GetParameterString)
                    ->Event("GetParameterVector2", &AnimGraphComponentRequestBus::Events::GetParameterVector2)
                    ->Event("GetParameterVector3", &AnimGraphComponentRequestBus::Events::GetParameterVector3)
                    ->Event("GetParameterRotationEuler", &AnimGraphComponentRequestBus::Events::GetParameterRotationEuler)
                    ->Event("GetParameterRotation", &AnimGraphComponentRequestBus::Events::GetNamedParameterRotation)
                    ->Event("GetNamedParameterFloat", &AnimGraphComponentRequestBus::Events::GetNamedParameterFloat)
                    ->Event("GetNamedParameterBool", &AnimGraphComponentRequestBus::Events::GetNamedParameterBool)
                    ->Event("GetNamedParameterString", &AnimGraphComponentRequestBus::Events::GetNamedParameterString)
                    ->Event("GetNamedParameterVector2", &AnimGraphComponentRequestBus::Events::GetNamedParameterVector2)
                    ->Event("GetNamedParameterVector3", &AnimGraphComponentRequestBus::Events::GetNamedParameterVector3)
                    ->Event("GetNamedParameterRotationEuler", &AnimGraphComponentRequestBus::Events::GetNamedParameterRotationEuler)
                    ->Event("GetNamedParameterRotation", &AnimGraphComponentRequestBus::Events::GetNamedParameterRotation)
                    ->Event("GetVisualizeEnabled", &AnimGraphComponentRequestBus::Events::GetVisualizeEnabled)
                    // Anim Graph Sync
                    ->Event("SyncAnimGraph", &AnimGraphComponentRequestBus::Events::SyncAnimGraph)
                    ->Event("DesyncAnimGraph", &AnimGraphComponentRequestBus::Events::DesyncAnimGraph)

                    // Motion sets
                    ->Event("SetMotionSet", &AnimGraphComponentRequestBus::Events::SetMotionSet)
                ;

				behaviorContext->EBus<AnimGraphNodeRequestBus>("AnimGraphNodeRequestBus")
					// General API
					->Event("GetNodeIdByName", &AnimGraphNodeRequestBus::Events::GetNodeIdByName)
					// Getters
					->Event("GetDuration", &AnimGraphNodeRequestBus::Events::GetDuration)
					->Event("GetPlaytime", &AnimGraphNodeRequestBus::Events::GetPlaytime)
					->Event("GetPlayspeed", &AnimGraphNodeRequestBus::Events::GetPlayspeed)
					// Setters
					->Event("SetPlayTime", &AnimGraphNodeRequestBus::Events::SetPlayTime)
					->Event("SetPlaySpeed", &AnimGraphNodeRequestBus::Events::SetPlaySpeed)
					// Special Functions
					->Event("Rewind", &AnimGraphNodeRequestBus::Events::Rewind)
					->Event("GetActiveNodes", &AnimGraphNodeRequestBus::Events::GetActiveNodes)
					;

                behaviorContext->EBus<AnimGraphComponentNotificationBus>("AnimGraphComponentNotificationBus")
                    ->Handler<AnimGraphComponentNotificationBehaviorHandler>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::List)
                ;

                behaviorContext->EBus<AnimGraphComponentNetworkRequestBus>("AnimGraphComponentNetworkRequestBus")
                    ->Attribute(AZ::Script::Attributes::Category, "Animation")
                    ->Event("IsAssetReady", &AnimGraphComponentNetworkRequestBus::Events::IsAssetReady)
                    ->Event("HasSnapshot", &AnimGraphComponentNetworkRequestBus::Events::HasSnapshot)
                    ->Event("CreateSnapshot", &AnimGraphComponentNetworkRequestBus::Events::CreateSnapshot)
                    ->Event("SetActiveStates", &AnimGraphComponentNetworkRequestBus::Events::SetActiveStates)
                    ->Event("GetActiveStates", &AnimGraphComponentNetworkRequestBus::Events::GetActiveStates)
                ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AnimGraphComponent::AnimGraphComponent(const Configuration* config)
        {
            if (config)
            {
                m_configuration = *config;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AnimGraphComponent::~AnimGraphComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::Init()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::Activate()
        {
            m_nextMotionSetStackToken = 1;
            m_motionSetStack.clear();

            m_animGraphInstance.reset();

            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            auto& cfg = m_configuration;
            if (cfg.m_animGraphAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(cfg.m_animGraphAsset.GetId());
                cfg.m_animGraphAsset.QueueLoad();
                cfg.m_animGraphAsset.BlockUntilLoadComplete();

                if (cfg.m_motionSetAsset.GetId().IsValid())
                {
                    AZ::Data::AssetBus::MultiHandler::BusConnect(cfg.m_motionSetAsset.GetId());
                    cfg.m_motionSetAsset.QueueLoad();
                    cfg.m_motionSetAsset.BlockUntilLoadComplete();
                }
            }

            ActorComponentNotificationBus::Handler::BusConnect(GetEntityId());
            AnimGraphComponentRequestBus::Handler::BusConnect(GetEntityId());
			AnimGraphNodeRequestBus::Handler::BusConnect(GetEntityId());
            AnimGraphComponentNotificationBus::Handler::BusConnect(GetEntityId());
            AnimGraphComponentNetworkRequestBus::Handler::BusConnect(GetEntityId());


            if (!m_actorInstance)
            {
                AZ_Error("AnimGraphComponent", false, "Failed to get actor instance for entity \"%s\" %s.",
                         GetEntity()->GetName().c_str(),
                         GetEntityId().ToString().c_str());
                return;
            }

            if (!m_animGraphInstance)
            {
                AZ_Error("AnimGraphComponent", false, "Failed to create anim graph instance for entity \"%s\" %s.",
                         GetEntity()->GetName().c_str(),
                         GetEntityId().ToString().c_str());
                return;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::Deactivate()
        {
            AnimGraphComponentNetworkRequestBus::Handler::BusDisconnect();
            AnimGraphComponentNotificationBus::Handler::BusDisconnect();
            AnimGraphComponentRequestBus::Handler::BusDisconnect();
            ActorComponentNotificationBus::Handler::BusDisconnect();
			AnimGraphNodeRequestBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            m_actorInstance.reset();
            DestroyAnimGraphInstance();
            m_configuration.m_animGraphAsset.Release();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnAssetReady(asset);
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            auto& cfg = m_configuration;

            // Keep the previous asset around until the anim graph instances are removed
            AZ::Data::Asset<AZ::Data::AssetData> prevAnimGraphAsset = cfg.m_animGraphAsset;
            AZ::Data::Asset<AZ::Data::AssetData> prevMotionSetAsset = cfg.m_motionSetAsset;
            if (asset == cfg.m_animGraphAsset)
            {
                cfg.m_animGraphAsset = asset;
            }
            else if (asset == cfg.m_motionSetAsset)
            {
                cfg.m_motionSetAsset = asset;
            }

            CheckCreateAnimGraphInstance();
        }

        void AnimGraphComponent::SetAnimGraphAssetId(const AZ::Data::AssetId& assetId)
        {
            m_configuration.m_animGraphAsset = AZ::Data::Asset<AnimGraphAsset>(assetId, azrtti_typeid<AnimGraphAsset>());
        }

        void AnimGraphComponent::SetMotionSetAssetId(const AZ::Data::AssetId& assetId)
        {
            m_configuration.m_motionSetAsset = AZ::Data::Asset<MotionSetAsset>(assetId, azrtti_typeid<MotionSetAsset>());
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance)
        {
            m_actorInstance = actorInstance;

            CheckCreateAnimGraphInstance();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::OnActorInstanceDestroyed(EMotionFX::ActorInstance* /*actorInstance*/)
        {
            DestroyAnimGraphInstance();

            m_actorInstance.reset();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::OnAnimGraphSynced(EMotionFX::AnimGraphInstance* animGraphInstance)
        {
            if (m_animGraphInstance)
            {
                m_animGraphInstance->AddFollowerGraph(animGraphInstance, true);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::OnAnimGraphDesynced(EMotionFX::AnimGraphInstance* animGraphInstance)
        {
            if (m_animGraphInstance)
            {
                m_animGraphInstance->RemoveFollowerGraph(animGraphInstance, true);
            }
        }

        bool AnimGraphComponent::IsAssetReady() const
        {
            return (m_actorInstance && m_animGraphInstance);
        }

        bool AnimGraphComponent::HasSnapshot() const
        {
            if (m_animGraphInstance && m_animGraphInstance->GetSnapshot())
            {
                return true;
            }
            return false;
        }

        void AnimGraphComponent::CreateSnapshot(bool isAuthoritative)
        {
            if (m_animGraphInstance)
            {
                m_animGraphInstance->CreateSnapshot(isAuthoritative);
                m_animGraphInstance->OnNetworkConnected();

                // This will stop the MCore Job schedule update the actor instance and anim graph for authoritative entity.
                // After doing so, we will have to update this actor manually in the networking update.
                m_animGraphInstance->GetActorInstance()->SetIsEnabled(!isAuthoritative);
            }
            else
            {
                AZ_Error("EMotionFX", false, "Cannot create snapshot as anim graph instance has not been created yet. "
                    "Please make sure you selected an anim graph in the anim graph component.");
            }
        }

        void AnimGraphComponent::SetActiveStates(const AZStd::vector<AZ::u32>& activeStates)
        {
            if (m_animGraphInstance)
            {
                m_animGraphInstance->OnNetworkActiveNodesUpdate(activeStates);
            }
        }

        void AnimGraphComponent::SetMotionPlaytimes(const MotionNodePlaytimeContainer& motionNodePlaytimes)
        {
            if (m_animGraphInstance)
            {
                m_animGraphInstance->OnNetworkMotionNodePlaytimesUpdate(motionNodePlaytimes);
            }
        }

        NodeIndexContainer AnimGraphComponent::s_emptyNodeIndexContainer = {};
        const NodeIndexContainer& AnimGraphComponent::GetActiveStates() const
        {
            if (m_animGraphInstance)
            {
                const AZStd::shared_ptr<AnimGraphSnapshot> snapshot = m_animGraphInstance->GetSnapshot();
                if (snapshot)
                {
                    AZ_Warning("EMotionFX", false, "Call GetActiveStates function but no snapshot is created for this instance.");
                    return snapshot->GetActiveNodes();
                }
            }

            return s_emptyNodeIndexContainer;
        }

        MotionNodePlaytimeContainer AnimGraphComponent::s_emptyMotionNodePlaytimeContainer = {};
        const MotionNodePlaytimeContainer& AnimGraphComponent::GetMotionPlaytimes() const
        {
            if (m_animGraphInstance)
            {
                const AZStd::shared_ptr<AnimGraphSnapshot> snapshot = m_animGraphInstance->GetSnapshot();
                if (snapshot)
                {
                    AZ_Warning("EMotionFX", false, "Call GetActiveStates function but no snapshot is created for this instance.");
                    return snapshot->GetMotionNodePlaytimes();
                }
            }

            return s_emptyMotionNodePlaytimeContainer;
        }

        void AnimGraphComponent::UpdateActorExternal(float deltatime)
        {
            if (m_actorInstance)
            {
                m_actorInstance->UpdateTransformations(deltatime);
            }
        }

        void AnimGraphComponent::SetNetworkRandomSeed(AZ::u64 seed)
        {
            if (m_animGraphInstance)
            {
                m_animGraphInstance->GetLcgRandom().SetSeed(seed);
            }
        }

        AZ::u64 AnimGraphComponent::GetNetworkRandomSeed() const
        {
            if (m_animGraphInstance)
            {
                return m_animGraphInstance->GetLcgRandom().GetSeed();
            }
            return 0;
        }

        void AnimGraphComponent::CheckCreateAnimGraphInstance()
        {
            auto& cfg = m_configuration;

            if (m_actorInstance && cfg.m_animGraphAsset.IsReady() && cfg.m_motionSetAsset.IsReady())
            {
                DestroyAnimGraphInstance();

                EMotionFX::MotionSet* rootMotionSet = cfg.m_motionSetAsset.Get()->m_emfxMotionSet.get();
                EMotionFX::MotionSet* motionSet = rootMotionSet;
                if (!cfg.m_activeMotionSetName.empty())
                {
                    motionSet = rootMotionSet->RecursiveFindMotionSetByName(cfg.m_activeMotionSetName);
                    if (!motionSet)
                    {
                        AZ_Warning("EMotionFX", false, "Failed to find motion set \"%s\" in motion set file %s.",
                            cfg.m_activeMotionSetName.c_str(),
                            rootMotionSet->GetName());

                        motionSet = rootMotionSet;
                    }
                }

                m_animGraphInstance = cfg.m_animGraphAsset.Get()->CreateInstance(m_actorInstance.get(), motionSet);
                if (!m_animGraphInstance)
                {
                    AZ_Error("EMotionFX", false, "Failed to create anim graph instance for entity \"%s\" %s.",
                        GetEntity()->GetName().c_str(),
                        GetEntityId().ToString().c_str());
                    return;
                }
                
                m_animGraphInstance->SetVisualizationEnabled(cfg.m_visualize);

                m_actorInstance->SetAnimGraphInstance(m_animGraphInstance.get());

                AnimGraphInstancePostCreate();

                // Apply parameter defaults.
                for (AZ::ScriptProperty* parameter : cfg.m_parameterDefaults.m_parameters)
                {
                    const char* paramName = parameter->m_name.c_str();

                    if (azrtti_istypeof<AZ::ScriptPropertyNumber>(parameter))
                    {
                        // This will handle float and integer types.
                        SetNamedParameterFloat(paramName, aznumeric_caster(static_cast<AZ::ScriptPropertyNumber*>(parameter)->m_value));
                    }
                    else if (azrtti_istypeof<AZ::ScriptPropertyBoolean>(parameter))
                    {
                        SetNamedParameterBool(paramName, static_cast<AZ::ScriptPropertyBoolean*>(parameter)->m_value);
                    }
                    else if (azrtti_istypeof<AZ::ScriptPropertyString>(parameter))
                    {
                        SetNamedParameterString(paramName, static_cast<AZ::ScriptPropertyString*>(parameter)->m_value.c_str());
                    }
                    else
                    {
                        AZ_Warning("EMotionFX", false, "Invalid type for anim graph parameter \"%s\".", paramName);
                    }
                }

                // Notify listeners that the anim graph is ready.
                AnimGraphComponentNotificationBus::Event(
                    GetEntityId(),
                    &AnimGraphComponentNotificationBus::Events::OnAnimGraphInstanceCreated,
                    m_animGraphInstance.get());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::DestroyAnimGraphInstance()
        {
            if (m_animGraphInstance)
            {
                AnimGraphComponentNotificationBus::Event(
                    GetEntityId(),
                    &AnimGraphComponentNotificationBus::Events::OnAnimGraphInstanceDestroyed,
                    m_animGraphInstance.get());

                AnimGraphInstancePreDestroy();

                m_animGraphInstance.reset();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::AnimGraphInstancePostCreate()
        {
            // Reference is not incremented when the instance is assigned to the actor, but is
            // decremented when actor is destroyed. Add a ref here to account for this.
            m_animGraphInstance.get()->IncreaseReferenceCount();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::AnimGraphInstancePreDestroy()
        {
            // If the anim graph is still active on the actor, deactivate it.
            // Also remove the extra reference we added to account for the actor's ownership
            // over it (see corresponding logic in OnAnimGraphInstanceCreated()), since we're
            // relinquishing that ownership.
            if (m_actorInstance && m_animGraphInstance && m_actorInstance->GetAnimGraphInstance() == m_animGraphInstance.get())
            {
                m_actorInstance->SetAnimGraphInstance(nullptr);
                m_animGraphInstance->DecreaseReferenceCount();
            }
        }

        AnimGraphPlaySpeedModifierPtr AnimGraphComponent::AddPlayspeedModifier(float modifier)
        {
            return m_animGraphInstance ? m_animGraphInstance->AddPlayspeedModifier(modifier) : AnimGraphPlaySpeedModifierPtr();
        }

        EMotionFX::AnimGraphInstance* AnimGraphComponent::GetAnimGraphInstance()
        {
            return m_animGraphInstance ? m_animGraphInstance.get() : nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        size_t AnimGraphComponent::FindParameterIndex(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex.IsSuccess())
                {
                    return parameterIndex.GetValue();
                }
            }

            return InvalidIndex;
        }

        void AnimGraphComponent::SetParameter(size_t parameterIndex, AZStd::any value) {
            using namespace AZStd;

            if (parameterIndex == InvalidIndex) {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return;
            }

            if (!m_animGraphInstance) return;

            MCore::Attribute* param = m_animGraphInstance->GetParameterValue(parameterIndex);
            if (!param) { 
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return;
            }

            AZStd::any previousVal; param->ToAny(previousVal);
            if (!param->FromAny(value)) { 
                AZ_Warning("EMotionFX", false, "Failed to set parameter at index: %u", parameterIndex);
                return;
            }

            SendParameterChangeEvent(previousVal, value, parameterIndex);
        }

        AZStd::any AnimGraphComponent::GetParameter(size_t parameterIndex) {
            AZStd::any val;
            if (parameterIndex == InvalidIndex) {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return val;
            }
            if (!m_animGraphInstance) return val;

            MCore::Attribute* param = m_animGraphInstance->GetParameterValue(parameterIndex);
            if (!param) { 
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return val;
            }

            if (!param->ToAny(val)) { 
                AZ_Warning("EMotionFX", false, "Failed to get parameter at index: %u", parameterIndex);
                return val;
            }

            return val;
        }

        void AnimGraphComponent::SetNamedParameter(const char * parameterName, AZStd::any value) {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (!parameterIndex.IsSuccess())
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameter(parameterIndex.GetValue(), value);
            }
        }

        AZStd::any AnimGraphComponent::GetNamedParameter(const char * parameterName) {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex.IsSuccess())
                {
                    return GetParameter(parameterIndex.GetValue());
                }
            }
            return AZStd::any();
        }

        //////////////////////////////////////////////////////////////////////////

        AZ_FORCE_INLINE void AnimGraphComponent::SendParameterChangeEvent(const AZStd::any& prev, const AZStd::any& current, size_t parameterIndex) {
            SendGenericAnimGraphParameterChangeEvent(GetEntityId(), m_animGraphInstance.get(), prev, current, parameterIndex);
        }

        template<class Type>
        inline Type any_generic_cast (const AZStd::any& anyVal) { 
            Type realVal;
            if (!MCore::Attribute_impl::any_generic_cast(anyVal, realVal)) { 
                AZ_Error("EmotionFX", false, "Could not cast to required type");
            }
            return realVal;
        }

        void SendGenericAnimGraphParameterChangeEvent(AZ::EntityId entityId, AnimGraphInstance* animGraphInstance, const AZStd::any& prev, const AZStd::any& current, AZ::u32 parameterIndex)
        {
#define ANIMGRAPH_SENDPARAMETERCHANGEEVENT(Ev, Ty)\
    EBUS_EVENT_ID(entityId, AnimGraphComponentNotificationBus, OnAnimGraph##Ev##ParameterChanged,\
                  animGraphInstance,\
                  parameterIndex,\
                  any_generic_cast<Ty>(prev),\
                  any_generic_cast<Ty>(current));\

            EBUS_EVENT_ID(entityId, AnimGraphComponentNotificationBus, OnAnimGraphParameterChanged,
                          animGraphInstance,
                          parameterIndex,
                          prev,
                          current);

            MCore::Attribute* param = animGraphInstance->GetParameterValue(parameterIndex);
            auto paramType = param->GetType();

            switch (paramType) {
            case MCore::AttributeFloat::TYPE_ID:
            {
                ANIMGRAPH_SENDPARAMETERCHANGEEVENT(Float, float);
                break;
            }
            case MCore::AttributeBool::TYPE_ID:
            {
                ANIMGRAPH_SENDPARAMETERCHANGEEVENT(Bool, bool);
                break;
            }
            case MCore::AttributeInt32::TYPE_ID:
            {
                ANIMGRAPH_SENDPARAMETERCHANGEEVENT(Float, float);
                break;
            }
            case MCore::AttributeColor::TYPE_ID:
            {
                // No event for this type
                break;
            }
            case MCore::AttributeQuaternion::TYPE_ID:
            {
                AZ::Quaternion current_;
                if (current.is<AZ::Vector3>()) {
                    current_.SetFromEulerRadians(any_generic_cast<AZ::Vector3>(current));
                } else {
                    current_ = AZStd::any_cast<AZ::Quaternion>(current);
                }

                EBUS_EVENT_ID(entityId, AnimGraphComponentNotificationBus, OnAnimGraphRotationParameterChanged,
                              animGraphInstance,
                              parameterIndex,
                              any_generic_cast<AZ::Quaternion>(prev),
                              current_);
                break;
            }
            case MCore::AttributeString::TYPE_ID:
            {
                EBUS_EVENT_ID(entityId, AnimGraphComponentNotificationBus, OnAnimGraphStringParameterChanged,
                  animGraphInstance,
                  parameterIndex,
                  any_generic_cast<AZStd::string>(prev).c_str(),
                  any_generic_cast<AZStd::string>(current).c_str());
                break;
            }
            case MCore::AttributeVector2::TYPE_ID:
            {
                ANIMGRAPH_SENDPARAMETERCHANGEEVENT(Vector2, AZ::Vector2);
                break;
            }
            case MCore::AttributeVector3::TYPE_ID:
            {
                ANIMGRAPH_SENDPARAMETERCHANGEEVENT(Vector3, AZ::Vector3);
                break;
            }
            case MCore::AttributeVector4::TYPE_ID:
            {
                // No event for this type
                break;
            }
            default:
            break;
            }
#undef ANIMGRAPH_SENDPARAMETERCHANGEEVENT
        }

        //////////////////////////////////////////////////////////////////////////
        const char* AnimGraphComponent::FindParameterName(size_t parameterIndex)
        {
            if (parameterIndex == InvalidIndex || !m_animGraphInstance || !m_animGraphInstance->GetAnimGraph())
            {
                return "";
            }
            return m_animGraphInstance->GetAnimGraph()->FindParameter(parameterIndex)->GetName().c_str();
        }


        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterFloat(size_t parameterIndex, float value)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::Attribute* param = m_animGraphInstance->GetParameterValue(parameterIndex);
                float previousValue;

                switch (param->GetType())
                {
                case MCore::AttributeFloat::TYPE_ID:
                {
                    MCore::AttributeFloat* floatParam = static_cast<MCore::AttributeFloat*>(param);
                    previousValue = floatParam->GetValue();
                    floatParam->SetValue(value);
                    break;
                }
                case MCore::AttributeBool::TYPE_ID:
                {
                    MCore::AttributeBool* boolParam = static_cast<MCore::AttributeBool*>(param);
                    previousValue = boolParam->GetValue();
                    boolParam->SetValue(!MCore::Math::IsFloatZero(value));
                    break;
                }
                case MCore::AttributeInt32::TYPE_ID:
                {
                    MCore::AttributeInt32* intParam = static_cast<MCore::AttributeInt32*>(param);
                    previousValue = static_cast<float>(intParam->GetValue());
                    intParam->SetValue(static_cast<int32>(value));
                    break;
                }
                default:
                {
                    AZ_Warning("EMotionFX", false, "Anim graph parameter index: %zu can not be set as float, is of type: %s", parameterIndex, param->GetTypeString());
                    return;
                }
                }

                // Notify listeners about the parameter change
                AnimGraphComponentNotificationBus::Event(
                    GetEntityId(),
                    &AnimGraphComponentNotificationBus::Events::OnAnimGraphFloatParameterChanged,
                    m_animGraphInstance.get(),
                    parameterIndex,
                    previousValue,
                    value);

                EBUS_EVENT_ID(GetEntityId(), AnimGraphComponentNotificationBus, OnAnimGraphParameterChanged, 
                              m_animGraphInstance.get(),
                              parameterIndex,
                              AZStd::any(previousValue),
                              AZStd::any(value));
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterBool(size_t parameterIndex, bool value)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::Attribute* param = m_animGraphInstance->GetParameterValue(parameterIndex);
                bool previousValue;

                switch (param->GetType())
                {
                case MCore::AttributeBool::TYPE_ID:
                {
                    MCore::AttributeBool* boolParam = static_cast<MCore::AttributeBool*>(param);
                    previousValue = boolParam->GetValue();
                    boolParam->SetValue(value);
                    break;
                }
                case MCore::AttributeFloat::TYPE_ID:
                {
                    MCore::AttributeFloat* floatParam = static_cast<MCore::AttributeFloat*>(param);
                    previousValue = !MCore::Math::IsFloatZero(floatParam->GetValue());
                    floatParam->SetValue(value);
                    break;
                }
                case MCore::AttributeInt32::TYPE_ID:
                {
                    MCore::AttributeInt32* intParam = static_cast<MCore::AttributeInt32*>(param);
                    previousValue = intParam->GetValue() != 0;
                    intParam->SetValue(value);
                    break;
                }
                default:
                {
                    AZ_Warning("EMotionFX", false, "Anim graph parameter index: %zu can not be set as bool, is of type: %s", parameterIndex, param->GetTypeString());
                    return;
                }
                }

                EBUS_EVENT_ID(GetEntityId(), AnimGraphComponentNotificationBus, OnAnimGraphParameterChanged, 
                              m_animGraphInstance.get(),
                              parameterIndex,
                              AZStd::any(previousValue),
                              AZStd::any(value));

                // Notify listeners about the parameter change
                AnimGraphComponentNotificationBus::Event(
                    GetEntityId(),
                    &AnimGraphComponentNotificationBus::Events::OnAnimGraphBoolParameterChanged,
                    m_animGraphInstance.get(),
                    parameterIndex,
                    previousValue,
                    value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterString(size_t parameterIndex, const char* cStringValue)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeString* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeString>(parameterIndex);
                if (param)
                {
                    // Since the event is sent out synchronously we just need to keep a copy of the
                    // previous value. The new value can be reused from "value"
                    // If the event were to change to a queued event, the parameters should be changed
                    // to AZStd::string for safety.
                    const AZStd::string previousValue(param->GetValue());
                    const AZStd::string currentValue(cStringValue);
                    param->SetValue(currentValue);

                    // Notify listeners about the parameter change
                    AnimGraphComponentNotificationBus::Event(
                        GetEntityId(),
                        &AnimGraphComponentNotificationBus::Events::OnAnimGraphStringParameterChanged,
                        m_animGraphInstance.get(),
                        parameterIndex,
                        previousValue.c_str(),
                        cStringValue);

                    EBUS_EVENT_ID(GetEntityId(), AnimGraphComponentNotificationBus, OnAnimGraphParameterChanged, 
                                  m_animGraphInstance.get(),
                                  parameterIndex,
                                  AZStd::any(previousValue),
                                  AZStd::any(currentValue));
                }
                else
                {
                    AZ_Warning("EMotionFX", false, "Anim graph parameter index: %zu is not a string", parameterIndex);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterVector2(size_t parameterIndex, const AZ::Vector2& value)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeVector2* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeVector2>(parameterIndex);
                if (param)
                {
                    const AZ::Vector2 previousValue = param->GetValue();
                    param->SetValue(value);

                    // Notify listeners about the parameter change
                    AnimGraphComponentNotificationBus::Event(
                        GetEntityId(),
                        &AnimGraphComponentNotificationBus::Events::OnAnimGraphVector2ParameterChanged,
                        m_animGraphInstance.get(),
                        parameterIndex,
                        previousValue,
                        value);

                    EBUS_EVENT_ID(GetEntityId(), AnimGraphComponentNotificationBus, OnAnimGraphParameterChanged, 
                                  m_animGraphInstance.get(),
                                  parameterIndex,
                                  AZStd::any(previousValue),
                                  AZStd::any(value));
                }
                else
                {
                    AZ_Warning("EMotionFX", false, "Anim graph parameter index: %zu is not a vector2", parameterIndex);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterVector3(size_t parameterIndex, const AZ::Vector3& value)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeVector3* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeVector3>(parameterIndex);
                if (param)
                {
                    const AZ::Vector3 previousValue = param->GetValue();
                    param->SetValue(value);

                    // Notify listeners about the parameter change
                    AnimGraphComponentNotificationBus::Event(
                        GetEntityId(),
                        &AnimGraphComponentNotificationBus::Events::OnAnimGraphVector3ParameterChanged,
                        m_animGraphInstance.get(),
                        parameterIndex,
                        previousValue,
                        value);

                    EBUS_EVENT_ID(GetEntityId(), AnimGraphComponentNotificationBus, OnAnimGraphParameterChanged, 
                                  m_animGraphInstance.get(),
                                  parameterIndex,
                                  AZStd::any(previousValue),
                                  AZStd::any(value));
                }
                else
                {
                    AZ_Warning("EMotionFX", false, "Anim graph parameter index: %zu is not a vector3", parameterIndex);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterRotationEuler(size_t parameterIndex, const AZ::Vector3& value)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::Attribute* param = m_animGraphInstance->GetParameterValue(parameterIndex);
                AZ::Quaternion previousValue;

                switch (param->GetType())
                {
                case MCore::AttributeQuaternion::TYPE_ID:
                {
                    MCore::AttributeQuaternion* quaternionParam = static_cast<MCore::AttributeQuaternion*>(param);
                    previousValue = quaternionParam->GetValue();
                    quaternionParam->SetValue(MCore::AzEulerAnglesToAzQuat(value));
                    break;
                }
                default:
                    AZ_Warning("EMotionFX", false, "Anim graph parameter index: %zu can not be set as rotation euler, is of type: %s", parameterIndex, param->GetTypeString());
                    return;
                }

                auto value_quat = MCore::AzEulerAnglesToAzQuat(value);
                // Notify listeners about the parameter change
                AnimGraphComponentNotificationBus::Event(
                    GetEntityId(),
                    &AnimGraphComponentNotificationBus::Events::OnAnimGraphRotationParameterChanged,
                    m_animGraphInstance.get(),
                    parameterIndex,
                    previousValue,
                    value_quat);

                EBUS_EVENT_ID(GetEntityId(), AnimGraphComponentNotificationBus, OnAnimGraphParameterChanged, 
                              m_animGraphInstance.get(),
                              parameterIndex,
                              AZStd::any(previousValue),
                              AZStd::any(value_quat));
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterRotation(size_t parameterIndex, const AZ::Quaternion& value)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::Attribute* param = m_animGraphInstance->GetParameterValue(parameterIndex);
                AZ::Quaternion previousValue;

                switch (param->GetType())
                {
                case MCore::AttributeQuaternion::TYPE_ID:
                {
                    MCore::AttributeQuaternion* quaternionParam = static_cast<MCore::AttributeQuaternion*>(param);
                    previousValue = quaternionParam->GetValue();
                    quaternionParam->SetValue(value);
                    break;
                }
                default:
                    AZ_Warning("EMotionFX", false, "Anim graph parameter index: %zu can not be set as rotation, is of type: %s", parameterIndex, param->GetTypeString());
                    return;
                }

                // Notify listeners about the parameter change
                AnimGraphComponentNotificationBus::Event(
                    GetEntityId(),
                    &AnimGraphComponentNotificationBus::Events::OnAnimGraphRotationParameterChanged,
                    m_animGraphInstance.get(),
                    parameterIndex,
                    previousValue,
                    value);

                EBUS_EVENT_ID(GetEntityId(), AnimGraphComponentNotificationBus, OnAnimGraphParameterChanged, 
                              m_animGraphInstance.get(),
                              parameterIndex,
                              AZStd::any(previousValue),
                              AZStd::any(value));
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterFloat(const char* parameterName, float value)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (!parameterIndex.IsSuccess())
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterFloat(static_cast<AZ::u32>(parameterIndex.GetValue()), value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterBool(const char* parameterName, bool value)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (!parameterIndex.IsSuccess())
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterBool(static_cast<size_t>(parameterIndex.GetValue()), value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterString(const char* parameterName, const char* value)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (!parameterIndex.IsSuccess())
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterString(static_cast<size_t>(parameterIndex.GetValue()), value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterVector2(const char* parameterName, const AZ::Vector2& value)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (!parameterIndex.IsSuccess())
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterVector2(static_cast<size_t>(parameterIndex.GetValue()), value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterVector3(const char* parameterName, const AZ::Vector3& value)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (!parameterIndex.IsSuccess())
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterVector3(static_cast<size_t>(parameterIndex.GetValue()), value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterRotationEuler(const char* parameterName, const AZ::Vector3& value)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (!parameterIndex.IsSuccess())
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterRotationEuler(static_cast<size_t>(parameterIndex.GetValue()), value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterRotation(const char* parameterName, const AZ::Quaternion& value)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (!parameterIndex.IsSuccess())
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterRotation(static_cast<size_t>(parameterIndex.GetValue()), value);
            }
        }

        void AnimGraphComponent::SetVisualizeEnabled(bool enabled)
        {
            if (m_animGraphInstance)
            {
                m_animGraphInstance->SetVisualizationEnabled(enabled);
            }
        }

        bool AnimGraphComponent::GetVisualizeEnabled()
        {
            if (m_animGraphInstance)
            {
                return m_animGraphInstance->GetVisualizationEnabled();
            }

            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        float AnimGraphComponent::GetParameterFloat(size_t parameterIndex)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return 0.f;
            }

            float value = 0.f;
            if (m_animGraphInstance)
            {
                m_animGraphInstance->GetParameterValueAsFloat(parameterIndex, &value);
            }
            return value;
        }

        //////////////////////////////////////////////////////////////////////////
        bool AnimGraphComponent::GetParameterBool(size_t parameterIndex)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return false;
            }

            bool value = false;
            if (m_animGraphInstance)
            {
                m_animGraphInstance->GetParameterValueAsBool(parameterIndex, &value);
            }
            return value;
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string AnimGraphComponent::GetParameterString(size_t parameterIndex)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return AZStd::string();
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeString* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeString>(parameterIndex);
                if (param)
                {
                    return AZStd::string(param->GetValue().c_str());
                }
            }
            return AZStd::string();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector2 AnimGraphComponent::GetParameterVector2(size_t parameterIndex)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return AZ::Vector2::CreateZero();
            }

            if (m_animGraphInstance)
            {
                AZ::Vector2 value;
                m_animGraphInstance->GetVector2ParameterValue(parameterIndex, &value);
                return value;
            }
            return AZ::Vector2::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector3 AnimGraphComponent::GetParameterVector3(size_t parameterIndex)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return AZ::Vector3::CreateZero();
            }

            if (m_animGraphInstance)
            {
                AZ::Vector3 value;
                m_animGraphInstance->GetVector3ParameterValue(parameterIndex, &value);
                return value;
            }
            return AZ::Vector3::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector3 AnimGraphComponent::GetParameterRotationEuler(size_t parameterIndex)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return AZ::Vector3::CreateZero();
            }

            if (m_animGraphInstance)
            {
                AZ::Quaternion value;
                m_animGraphInstance->GetRotationParameterValue(parameterIndex, &value);
                return MCore::AzQuaternionToEulerAngles(value);
            }
            return AZ::Vector3::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Quaternion AnimGraphComponent::GetParameterRotation(size_t parameterIndex)
        {
            if (parameterIndex == InvalidIndex)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %zu", parameterIndex);
                return AZ::Quaternion::CreateZero();
            }

            if (m_animGraphInstance)
            {
                AZ::Quaternion value;
                m_animGraphInstance->GetRotationParameterValue(parameterIndex, &value);
                return value;
            }
            return AZ::Quaternion::CreateIdentity();
        }

        //////////////////////////////////////////////////////////////////////////
        float AnimGraphComponent::GetNamedParameterFloat(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex.IsSuccess())
                {
                    return GetParameterFloat(static_cast<size_t>(parameterIndex.GetValue()));
                }
            }
            return 0.f;
        }

        //////////////////////////////////////////////////////////////////////////
        bool AnimGraphComponent::GetNamedParameterBool(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex.IsSuccess())
                {
                    return GetParameterBool(static_cast<size_t>(parameterIndex.GetValue()));
                }
            }
            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string AnimGraphComponent::GetNamedParameterString(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex.IsSuccess())
                {
                    return GetParameterString(static_cast<size_t>(parameterIndex.GetValue()));
                }
            }
            return AZStd::string();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector2 AnimGraphComponent::GetNamedParameterVector2(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex.IsSuccess())
                {
                    return GetParameterVector2(static_cast<size_t>(parameterIndex.GetValue()));
                }
            }
            return AZ::Vector2::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector3 AnimGraphComponent::GetNamedParameterVector3(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex.IsSuccess())
                {
                    return GetParameterVector3(static_cast<size_t>(parameterIndex.GetValue()));
                }
            }
            return AZ::Vector3::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector3 AnimGraphComponent::GetNamedParameterRotationEuler(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex.IsSuccess())
                {
                    return GetParameterRotationEuler(static_cast<size_t>(parameterIndex.GetValue()));
                }
            }
            return AZ::Vector3::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Quaternion AnimGraphComponent::GetNamedParameterRotation(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex.IsSuccess())
                {
                    return GetParameterRotation(static_cast<size_t>(parameterIndex.GetValue()));
                }
            }
            return AZ::Quaternion::CreateIdentity();
        }

		AZ::u64 AnimGraphComponent::GetNodeIdByName(const char* nodeName)
		{
			if (m_animGraphInstance)
			{
				AnimGraphNode* node = m_animGraphInstance->GetAnimGraph()->RecursiveFindNodeByName(nodeName);
				if (node)
				{
					return node->GetId();
				}
			}
			return AnimGraphNodeId();
		}

		//////////////////////////////////////////////////////////////////////////
		float AnimGraphComponent::GetDuration(const char* nodeName)
		{
			if (m_animGraphInstance)
			{
				AnimGraphNode* node = m_animGraphInstance->GetAnimGraph()->RecursiveFindNodeByName(nodeName);
				if (node)
				{
					return node->GetDuration(m_animGraphInstance.get());
				}
			}
			return -1;
		}

		//////////////////////////////////////////////////////////////////////////
		float AnimGraphComponent::GetPlaytime(const char* nodeName)
		{
			if (m_animGraphInstance)
			{
				AnimGraphNode* node = m_animGraphInstance->GetAnimGraph()->RecursiveFindNodeByName(nodeName);
				if (node)
				{
					return node->GetCurrentPlayTime(m_animGraphInstance.get());
				}
			}
			return -1;
		}

		//////////////////////////////////////////////////////////////////////////
		float AnimGraphComponent::GetPlayspeed(const char* nodeName)
		{
			if (m_animGraphInstance)
			{
				AnimGraphNode* node = m_animGraphInstance->GetAnimGraph()->RecursiveFindNodeByName(nodeName);
				if (node)
				{
					return node->GetPlaySpeed(m_animGraphInstance.get());
				}
			}
			return -1;
		}

		//////////////////////////////////////////////////////////////////////////
		void AnimGraphComponent::SetPlayTime(const char* nodeName, float playtime)
		{
			if (m_animGraphInstance && playtime)
			{
				AnimGraphNode* node = m_animGraphInstance->GetAnimGraph()->RecursiveFindNodeByName(nodeName);
				if (node)
				{
					node->SetCurrentPlayTime(m_animGraphInstance.get(), playtime);
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		void AnimGraphComponent::SetPlaySpeed(const char* nodeName, float speed)
		{
			if (m_animGraphInstance && speed)
			{
				AnimGraphNode* node = m_animGraphInstance->GetAnimGraph()->RecursiveFindNodeByName(nodeName);
				if (node)
				{
					node->SetPlaySpeed(m_animGraphInstance.get(), speed);
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		void AnimGraphComponent::Rewind(const char* nodeName)
		{
			if (m_animGraphInstance)
			{
				AnimGraphNode* node = m_animGraphInstance->GetAnimGraph()->RecursiveFindNodeByName(nodeName);
				if (node)
				{
					node->Rewind(m_animGraphInstance.get());
				}
			}
		}

        //////////////////////////////////////////////////////////////////////////
        AZStd::vector<AZStd::string_view> AnimGraphComponent::GetActiveNodes(const AZ::TypeId& nodeType)
        {
            AZStd::vector<AZStd::string_view> res;
            if (m_animGraphInstance)
            {
                AZStd::vector<AnimGraphNode*> activeNodes;
                m_animGraphInstance->CollectActiveAnimGraphNodes(&activeNodes, nodeType);

                res.reserve(activeNodes.size());
                for (AnimGraphNode* activeNode : activeNodes)
                {
                    res.push_back(activeNode->GetName());
                }
            }

            return res;
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SyncAnimGraph(AZ::EntityId leaderEntityId)
        {
            if (m_animGraphInstance)
            {
                AnimGraphComponentNotificationBus::Event(
                    leaderEntityId,
                    &AnimGraphComponentNotificationBus::Events::OnAnimGraphSynced,
                    m_animGraphInstance.get());
            }
        }

        void AnimGraphComponent::DesyncAnimGraph(AZ::EntityId leaderEntityId)
        {
            if (m_animGraphInstance)
            {
                AnimGraphComponentNotificationBus::Event(
                    leaderEntityId,
                    &AnimGraphComponentNotificationBus::Events::OnAnimGraphDesynced,
                    m_animGraphInstance.get());
            }
        }

        void AnimGraphComponent::SetTopMotionSet()
        {
            if (m_animGraphInstance)
            {
                if (const auto currentMotionSet = m_animGraphInstance->GetMotionSet())
                {
                    const auto rootMotionSet = currentMotionSet->FindRootMotionSet();
                    AZ_Assert(rootMotionSet, AZ_FUNCTION_SIGNATURE " - root motion set is null?");

                    // Get motion set to activate - if the motionset stack is empty, use the root motion set
                    const auto newMotionSet
                        = m_motionSetStack.empty()
                        ? rootMotionSet
                        : rootMotionSet->RecursiveFindMotionSetByName(m_motionSetStack.rbegin()->second)
                        ;

                    if (newMotionSet)
                    {
                        if (newMotionSet != currentMotionSet)
                        {
                            m_animGraphInstance->SetMotionSet(newMotionSet);
                        }
                    }
                    else
                    {
                        AZ_Warning("EMotionFX", false, "Failed to find motion set \"%s\" in motion set file %s.",
                            m_motionSetStack.rbegin()->second.c_str(),
                            rootMotionSet->GetName());
                    }
                }
            }
        }

        AZStd::string AnimGraphComponent::GetMotionSet()
        {
            if (m_animGraphInstance)
            {
                if (const auto currentMotionSet = m_animGraphInstance->GetMotionSet())
                {
                    return currentMotionSet->GetNameString();
                }
            }

            return "";
        }

        void AnimGraphComponent::SetMotionSet(const AZStd::string& motionSetName)
        {
            PushMotionSet(motionSetName);
        }

        AZ::u64 AnimGraphComponent::PushMotionSet(const AZStd::string& motionSetName)
        {
            AZ::u64 token = 0;
            if (m_animGraphInstance)
            {
                token = m_nextMotionSetStackToken;
                m_nextMotionSetStackToken = m_nextMotionSetStackToken == std::numeric_limits<size_t>::max() ? 1 : m_nextMotionSetStackToken + 1;

                m_motionSetStack.emplace(token, motionSetName);
                SetTopMotionSet();
            }

            return token;
        }

        void AnimGraphComponent::PopMotionSet(AZ::u64 token)
        {
            m_motionSetStack.erase(token);
            SetTopMotionSet();
        }

    } // namespace Integration
} // namespace EMotionFXAnimation
