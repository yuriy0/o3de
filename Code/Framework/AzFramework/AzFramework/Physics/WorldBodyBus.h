#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBodyEvents.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzCore/Component/TickBus.h>

namespace Physics
{
    using WorldBodyRequestBus = AzPhysics::SimulatedBodyComponentRequestsBus;
    using WorldBodyRequests = AzPhysics::SimulatedBodyComponentRequestsBus::Events;
    
    //! Notifications for generic physical world bodies
    class WorldBodyNotifications
        : public AZ::ComponentBus
    {
    private:
        AzPhysics::SceneEvents::OnSimulationBodyAdded::Handler m_onBodyAddedHandler;
        AzPhysics::SceneEvents::OnSimulationBodyRemoved::Handler m_onBodyRemovedHandler;

        inline void DoDisconnect()
        {
            m_onBodyAddedHandler.Disconnect();
            m_onBodyRemovedHandler.Disconnect();
        }

        void QueueOnPhysicsEnabledEvent(AZ::EntityId id);

        inline void OnBodyAdded(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle, AZ::EntityId id)
        {
            // Check if the body is the same one as the connecting ID, if so, signal
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                if (auto scene = physicsSystem->GetScene(sceneHandle))
                {
                    if (auto body = scene->GetSimulatedBodyFromHandle(bodyHandle))
                    {
                        if (body->GetEntityId() == id)
                        {
                            QueueOnPhysicsEnabledEvent(id);
                        }
                    }
                }
            }
        }

        inline void OnBodyRemoved(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle, AZ::EntityId id)
        {
            // Check if the body is the same one as the connecting ID, if so, signal
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                if (auto scene = physicsSystem->GetScene(sceneHandle))
                {
                    if (auto body = scene->GetSimulatedBodyFromHandle(bodyHandle))
                    {
                        if (body->GetEntityId() == id)
                        {
                            OnPhysicsDisabled();
                        }
                    }
                }
            }
        }

        inline void ConnectSceneHandlers(AzPhysics::SystemInterface* physicsSystem, AzPhysics::SceneHandle sceneHandle, AZ::EntityId id)
        {
            if (auto scene = physicsSystem->GetScene(sceneHandle))
            {
                // Re-connect logic
                m_onBodyAddedHandler.Disconnect();
                m_onBodyRemovedHandler.Disconnect();

                // Create handler methods
                m_onBodyAddedHandler = decltype(m_onBodyAddedHandler)([this, id](AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
                {
                    OnBodyAdded(sceneHandle, bodyHandle, id);
                });
                m_onBodyRemovedHandler = decltype(m_onBodyRemovedHandler)([this, id](AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
                {
                    OnBodyRemoved(sceneHandle, bodyHandle, id);
                });

                // Register handlers
                scene->RegisterSimulationBodyAddedHandler(m_onBodyAddedHandler);
                scene->RegisterSimulationBodyRemovedHandler(m_onBodyRemovedHandler);
            }
        }
        
        inline void DoConnect(const AZ::EntityId& id)
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                auto [sceneHandle, bodyHandle] = physicsSystem->FindAttachedBodyHandleFromEntityId(id);
                if (sceneHandle != AzPhysics::InvalidSceneHandle)
                {
                    // Queue an enabled event immediately since the body already exists
                    // Also connect handlers in case we also interested in disconnect events (no way to know)
                    QueueOnPhysicsEnabledEvent(id);
                    ConnectSceneHandlers(physicsSystem, sceneHandle, id);
                }
                else
                {
                    // If the entity has no physics body, wait for one to be created on the default scene
                    EBUS_EVENT_RESULT(sceneHandle, Physics::DefaultWorldBus, GetDefaultSceneHandle);
                    ConnectSceneHandlers(physicsSystem, sceneHandle, id);
                }
            }
        }
        
        
    public:
        WorldBodyNotifications() = default;
        virtual ~WorldBodyNotifications()
        {
            DoDisconnect();
        }

        // TODO: copy/move
        AZ_DISABLE_COPY_MOVE(WorldBodyNotifications);

        template<class Bus>
        struct ConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                // Connect AZ::Event
                handler->DoConnect(id);
            }

            static void Disconnect(typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::BusPtr& ptr)
            {
                // Disconnect AZ::Event
                handler->DoDisconnect();

                AZ::EBusConnectionPolicy<Bus>::Disconnect(context, handler, ptr);
            }
        };
        
        //! Notification for physics enabled
        virtual void OnPhysicsEnabled() = 0;
        //! Notification for physics disabled
        virtual void OnPhysicsDisabled() = 0;
    };
    using WorldBodyNotificationBus = AZ::EBus<WorldBodyNotifications>;


    inline void WorldBodyNotifications::QueueOnPhysicsEnabledEvent(AZ::EntityId id)
    {
        // Delay the event by a tick because the physics system sends OnSimulationBodyAdded
        // before the physics component is fully ready (i.e. before its connected to its request bus)
        AZ::TickBus::QueueFunction([id]()
        {
            EBUS_EVENT_ID(id, WorldBodyNotificationBus, OnPhysicsEnabled);
        });
    }
}
