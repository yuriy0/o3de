/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include "TagComponent.h"

namespace LmbrCentral
{
    // BehaviorContext TagComponentNotificationsBus forwarder
    class BehaviorTagComponentNotificationsBusHandler : public TagComponentNotificationsBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorTagComponentNotificationsBusHandler,"{7AEDC591-41AB-4E3B-87D2-03346154279D}",AZ::SystemAllocator,
            OnTagAdded, OnTagRemoved);

        void OnTagAdded(const Tag& tag) override
        {
            Call(FN_OnTagAdded, tag);
        }

        void OnTagRemoved(const Tag& tag) override
        {
            Call(FN_OnTagRemoved, tag);
        }
    };

    class BehaviorTagGlobalNotificationBusHandler : public TagGlobalNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorTagGlobalNotificationBusHandler, "{87E9363C-C346-4A1E-BCDA-37C0504B1985}", AZ::SystemAllocator,
            OnEntityTagAdded, OnEntityTagRemoved);

        void OnEntityTagAdded(const AZ::EntityId& entityId) override
        {
            Call(FN_OnEntityTagAdded, entityId);
        }

        void OnEntityTagRemoved(const AZ::EntityId& entityId) override
        {
            Call(FN_OnEntityTagRemoved, entityId);
        }
    };

	class BehaviorTagGeneralNotificationsBusHandler : public TagGeneralNotificationsBus::Handler, public AZ::BehaviorEBusHandler
	{
	public:
		AZ_EBUS_BEHAVIOR_BINDER(BehaviorTagGeneralNotificationsBusHandler, "{90D3DA34-8C71-41AC-B431-D5C20536259E}", AZ::SystemAllocator,
			OnTagGeneralAdded, OnTagGeneralRemoved);

		void OnTagGeneralAdded(const Tag& tag, const AZ::EntityId& entityId) override
		{
			Call(FN_OnTagGeneralAdded, tag, entityId);
		}

		void OnTagGeneralRemoved(const Tag& tag, const AZ::EntityId& entityId) override
		{
			Call(FN_OnTagGeneralRemoved, tag, entityId);
		}
	};

    class TagComponentBehaviorHelper
    {
    public:
        AZ_RTTI(TagComponentBehaviorHelper, "{9BE9EE51-3705-4C3F-B9F1-F799C628D76F}");

        virtual ~TagComponentBehaviorHelper() = default;

        static AZStd::vector< AZ::EntityId > FindTaggedEntities(const AZ::Crc32& tagName)
        {
            AZ::EBusAggregateResults<AZ::EntityId> aggregator;
            TagGlobalRequestBus::EventResult(aggregator, tagName, &TagGlobalRequests::RequestTaggedEntities);
            return aggregator.values;
        }
    };

    //=========================================================================
    // Component Descriptor
    //=========================================================================
    void TagComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TagComponent, AZ::Component>()
                ->Version(1)
                ->Field("Tags", &TagComponent::m_tags);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<TagComponentBehaviorHelper>("Tag Helper")
                ->Method("Get Entities by Tag", &TagComponentBehaviorHelper::FindTaggedEntities)
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay/Tag")
                ->Attribute(AZ::ScriptCanvasAttributes::FloatingFunction, 0)
                ;

            behaviorContext->EBus<TagComponentRequestBus>("TagComponentRequestBus")
                ->Event("HasTag", &TagComponentRequestBus::Events::HasTag)
                ->Event("AddTag", &TagComponentRequestBus::Events::AddTag)
                ->Event("RemoveTag", &TagComponentRequestBus::Events::RemoveTag)
                ;

            behaviorContext->EBus<TagGlobalRequestBus>("TagGlobalRequestBus")
                ->Event("RequestTaggedEntities", &TagGlobalRequestBus::Events::RequestTaggedEntities)
                ;
            
            behaviorContext->EBus<TagComponentNotificationsBus>("TagComponentNotificationsBus")
                ->Handler<BehaviorTagComponentNotificationsBusHandler>()
                ;

            behaviorContext->EBus<TagGlobalNotificationBus>("TagGlobalNotificationBus")
                ->Handler<BehaviorTagGlobalNotificationBusHandler>()
                ;

			behaviorContext->EBus<TagGeneralNotificationsBus>("TagGeneralNotificationBus")
				->Handler<BehaviorTagGeneralNotificationsBusHandler>()
				;
        }
    }

    void TagComponent::SendRemoveTagEvent(const Tag & tag) {
        EBUS_EVENT_ID(GetEntityId(), TagComponentNotificationsBus, OnTagRemoved, tag);
        EBUS_EVENT_ID(tag, TagGlobalNotificationBus, OnEntityTagRemoved, GetEntityId());
        EBUS_EVENT(TagGeneralNotificationsBus, OnTagGeneralRemoved, tag, GetEntityId());
    }

    void TagComponent::SendAddTagEvent(const Tag & tag) {
        EBUS_EVENT_ID(GetEntityId(), TagComponentNotificationsBus, OnTagAdded, tag);
        EBUS_EVENT_ID(tag, TagGlobalNotificationBus, OnEntityTagAdded, GetEntityId());
        EBUS_EVENT(TagGeneralNotificationsBus, OnTagGeneralAdded, tag, GetEntityId());
    }

    //=========================================================================
    // AZ::Component
    //=========================================================================
    void TagComponent::Activate()
    {
        // Connect to tag buses
        for (const Tag& tag : m_tags)
        {
            TagGlobalRequestBus::MultiHandler::BusConnect(tag);
        }
        TagComponentRequestBus::Handler::BusConnect(GetEntityId());

        // Send events
        for (const Tag& tag : m_tags)
        {
            SendAddTagEvent(tag);
        }
    }

    void TagComponent::Deactivate()
    {
        // Disconnect from tag buses
        TagComponentRequestBus::Handler::BusDisconnect();
        for (const Tag& tag : m_tags)
        {
            TagGlobalRequestBus::MultiHandler::BusDisconnect(tag);
        }

        // Send events
        for (const Tag& tag : m_tags)
        {
            SendRemoveTagEvent(tag);
        }
    }

    //=========================================================================
    // EditorTagComponent friend will call this
    //=========================================================================
    void TagComponent::EditorSetTags(Tags&& editorTagList)
    {
        m_tags = AZStd::move(editorTagList);
    }

    //=========================================================================
    // TagRequestBus
    //=========================================================================
    bool TagComponent::HasTag(const Tag& tag)
    {
        return m_tags.find(tag) != m_tags.end();
    }

    void TagComponent::AddTag(const Tag& tag)
    {
        if (m_tags.insert(tag).second)
        {
            TagGlobalRequestBus::MultiHandler::BusConnect(tag);
            SendAddTagEvent(tag);
        }
    }

    void TagComponent::AddTags(const Tags& tags)
    {
        for (const Tag& tag : tags)
        {
            AddTag(tag);
        }
    }

    void TagComponent::RemoveTag(const Tag& tag)
    {
        if (m_tags.erase(tag) > 0)
        {
            TagGlobalRequestBus::MultiHandler::BusDisconnect(tag);
            SendRemoveTagEvent(tag);
        }
    }

    void TagComponent::RemoveTags(const Tags& tags)
    {
        for (const Tag& tag : tags)
        {
            RemoveTag(tag);
        }
    }

} // namespace LmbrCentral
