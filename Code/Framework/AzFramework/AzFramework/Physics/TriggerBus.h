#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBodyEvents.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/SystemBus.h>

namespace Physics
{
    using TriggerEvent = AzPhysics::TriggerEvent;

    class TriggerNotifications
        : public AZ::ComponentBus
    {
        AzPhysics::SimulatedBodyEvents::OnTriggerEnter::Handler m_onTriggerEnterHandler;
        AzPhysics::SimulatedBodyEvents::OnTriggerExit::Handler m_onTriggerExitHandler;
        AzPhysics::SceneEvents::OnSimulationBodyAdded::Handler m_onBodyAddedHandler;

        inline void DoDisconnect()
        {
            m_onTriggerEnterHandler.Disconnect();
            m_onTriggerExitHandler.Disconnect();
            m_onBodyAddedHandler.Disconnect();
        }

        inline void DoConnect(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
        {
            // Re-connect logic
            m_onTriggerEnterHandler.Disconnect();
            m_onTriggerExitHandler.Disconnect();

            // Setup handler methods
            m_onTriggerEnterHandler = decltype(m_onTriggerEnterHandler)([this](AzPhysics::SimulatedBodyHandle, const TriggerEvent& ev) { OnTriggerEnter(ev); });
            m_onTriggerExitHandler = decltype(m_onTriggerExitHandler)([this](AzPhysics::SimulatedBodyHandle, const TriggerEvent& ev) { OnTriggerExit(ev); });

            AzPhysics::SimulatedBodyEvents::RegisterOnTriggerEnterHandler(sceneHandle, bodyHandle, m_onTriggerEnterHandler);
            AzPhysics::SimulatedBodyEvents::RegisterOnTriggerExitHandler(sceneHandle, bodyHandle, m_onTriggerExitHandler);
        }

        __declspec(noinline) void OnBodyAdded(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle, AZ::EntityId id)
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                if (auto scene = physicsSystem->GetScene(sceneHandle))
                {
                    if (auto body = scene->GetSimulatedBodyFromHandle(bodyHandle))
                    {
                        if (body->GetEntityId() == id)
                        {
                            DoConnect(sceneHandle, bodyHandle);
                            m_onBodyAddedHandler.Disconnect();
                        }
                    }
                }
            }
        }

        inline void DoConnect(const AZ::EntityId& id)
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                auto [sceneHandle, bodyHandle] = physicsSystem->FindAttachedBodyHandleFromEntityId(id);
                if (sceneHandle != AzPhysics::InvalidSceneHandle)
                {
                    DoConnect(sceneHandle, bodyHandle);
                }
                else
                {
                    // If the entity has no physics body, wait for one to be created on the default scene
                    EBUS_EVENT_RESULT(sceneHandle, Physics::DefaultWorldBus, GetDefaultSceneHandle);
                    if (auto scene = physicsSystem->GetScene(sceneHandle))
                    {
                        m_onBodyAddedHandler = decltype(m_onBodyAddedHandler)([this, id](AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
                        {
                            OnBodyAdded(sceneHandle, bodyHandle, id);
                        });

                        scene->RegisterSimulationBodyAddedHandler(m_onBodyAddedHandler);
                    }
                }
            }
        }

    public:
        TriggerNotifications() = default;
        virtual ~TriggerNotifications()
        {
            DoDisconnect();
        }

        // TODO: copy/move
        AZ_DISABLE_COPY_MOVE(TriggerNotifications);

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

        virtual void OnTriggerEnter([[maybe_unused]] const TriggerEvent& triggerEvent) {};
        virtual void OnTriggerExit([[maybe_unused]] const TriggerEvent& triggerEvent) {};
    };

    using TriggerNotificationBus = AZ::EBus<TriggerNotifications>;
}
