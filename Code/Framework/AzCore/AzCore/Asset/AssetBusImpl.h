/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
* Modifications copyright Apocalypse Studios Inc.
*/

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock_ext.h>
#include <mutex>

// Executes router handling in a generic way, this gets undefed in BusImpl.h
#define EBUS_DO_ROUTING(contextParam, id, isQueued, isReverse) \
    do {                                                                                        \
        auto& local_context = (contextParam);                                                   \
        if (local_context.m_routing.m_routers.size()) {                                         \
            if (local_context.m_routing.RouteEvent(id, isQueued, isReverse, func, args...)) {   \
                return;                                                                         \
            }                                                                                   \
        }                                                                                       \
    } while(false)

namespace AZ
{
    namespace Data
    {
        namespace AssetInternal
        {
            // A mutex type which is moveable, and satisfies BasicLockable
            template<class M>
            class moveable_mutex
            {
            public:
                moveable_mutex()
                    : m_mutex(new M)
                {}

                AZ_FORCE_INLINE void lock()
                {
                    return m_mutex->lock();
                }
                AZ_FORCE_INLINE bool try_lock()
                {
                    return m_mutex->try_lock();
                }
                AZ_FORCE_INLINE void unlock()
                {
                    return m_mutex->unlock();
                }

                const M& get_mutex() const { return *m_mutex; }
                M& get_mutex() { return *m_mutex; }

            private:
                AZStd::shared_ptr<M> m_mutex;
            };

            // The mutex type used to protect per-ID dispatches on the asset bus
            using AssetBusHandlerIdHandlerMutexT = moveable_mutex<AZStd::recursive_mutex>;

            template<class Interface>
            class AssetBusImpl;

            // Extends the default bus container handler holder to add the per-ID mutex
            template<class Interface, class Traits>
            struct AssetBusHandlerHolder : public AZ::EBusTraits::EBusContainer<Interface, Traits>::DefaultHandlerHolder
            {
                using BaseType = typename AZ::EBusTraits::EBusContainer<Interface, Traits>::DefaultHandlerHolder;
                using ContainerType = AZ::EBusTraits::EBusContainer<Interface, Traits>;
                using IdType = typename Traits::BusIdType;

                AssetBusHandlerIdHandlerMutexT m_handlerMutex;

                AssetBusHandlerHolder(ContainerType& storage, const IdType& id)
                    : BaseType(storage, id)
                {}

                AssetBusHandlerHolder(AssetBusHandlerHolder&& rhs)
                    : BaseType(static_cast<BaseType&&>(rhs))
                    , m_handlerMutex(AZStd::move(rhs.m_handlerMutex))
                {}

                AssetBusHandlerHolder(const AssetBusHandlerHolder&) = delete;
                AssetBusHandlerHolder& operator=(const AssetBusHandlerHolder&) = delete;
                AssetBusHandlerHolder& operator=(AssetBusHandlerHolder&&) = delete;
            };

            // Extends the default bus container with custom disconnect functions
            template<class Interface, class Traits>
            class AssetBusContainer : public AZ::EBusTraits::EBusContainer<Interface, Traits>
            {
                using Base = AZ::EBusTraits::EBusContainer<Interface, Traits>;
                using HandlerBase = typename Base::Handler;
                using MultiHandlerBase = typename Base::MultiHandler;
                using BusType = AssetBusImpl<Interface>;
                using CallstackEntry = typename Base::CallstackEntry;

            public:
                using HandlerHolder = typename Base::HandlerHolder;
                using HandlerNode = typename Base::HandlerNode;
                using IdType = typename Base::IdType;

                struct Handler : public HandlerBase
                {
                private:
                    template<class IsConnectedFn>
                    void BusDisconnect_Impl(IsConnectedFn&& isConnectedFn)
                    {
                        // Handlers lock to acquire before leaving this function;
                        // the dispatcher holds the lock while inside a handler method,
                        // and it's likely that the handler object will be deleted
                        // after calling BusDisconnect
                        AZStd::optional<AssetBusHandlerIdHandlerMutexT> handlersLockMutex;

                        if (typename BusType::Context* context = BusType::GetContext())
                        {
                            AZStd::scoped_lock<decltype(context->m_contextMutex)> contextLock(context->m_contextMutex);
                            if (isConnectedFn())
                            {
                                // We are about to remove this handler node, which would delete the handler lock,
                                // but another thread may still be inside dispatch and might be holding this handler lock.
                                // so take ownership of the lock now, before deleting the node
                                handlersLockMutex.emplace(HandlerBase::m_node.m_holder->m_handlerMutex);

                                BusType::DisconnectInternal(*context, HandlerBase::m_node);
                            }
                        }

                        // Lock the per ID handlers lock; if we are inside dispatch, this means this function
                        // will wait until dispatch is complete.
                        // Note that there is no guarantee of fairness: we may enter another dispatch immediately
                        // after the first; and the second dispatch might get the handlers lock before this function does
                        if (handlersLockMutex)
                        {
                            handlersLockMutex->lock();
                            handlersLockMutex->unlock();
                        }
                    }

                public:
                    void BusDisconnect(const IdType& id)
                    {
                        BusDisconnect_Impl([this, &id]() { return HandlerBase::BusIsConnectedId(id); });
                    }

                    void BusDisconnect()
                    {
                        BusDisconnect_Impl([this]() { return HandlerBase::BusIsConnected(); });
                    }
                };

                struct MultiHandler : public MultiHandlerBase
                {
                public:
                    void BusDisconnect(const IdType& id)
                    {
                        // Storage for the id handler mutex
                        AZStd::optional<AssetBusHandlerIdHandlerMutexT> handlersLockMutex;

                        if (typename BusType::Context* context = BusType::GetContext())
                        {
                            AZStd::scoped_lock<decltype(context->m_contextMutex)> contextLock(context->m_contextMutex);
                            auto& handlerNodes = MultiHandlerBase::m_handlerNodes;
                            auto nodeIt = handlerNodes.find(id);
                            if (nodeIt != handlerNodes.end())
                            {
                                HandlerNode* handlerNode = nodeIt->second;

                                // Store the id handler mutex, since the handler node is going to be deleted
                                if (handlerNode->m_holder)
                                {
                                    handlersLockMutex.emplace(handlerNode->m_holder->m_handlerMutex);
                                }

                                BusType::DisconnectInternal(*context, *handlerNode);
                                handlerNodes.erase(nodeIt);
                                handlerNode->~HandlerNode();
                                handlerNodes.get_allocator().deallocate(handlerNode, sizeof(HandlerNode), alignof(HandlerNode));
                            }
                        }

                        // Lock and unlock immediately the ID handler mutex; if inside dispatch, this prevents the destruction
                        // of the handler object until dispatch completes
                        if (handlersLockMutex)
                        {
                            handlersLockMutex->lock();
                            handlersLockMutex->unlock();
                        }
                    }

                    void BusDisconnect()
                    {
                        // Storage for the id handler mutexes
                        AZStd::vector<AssetBusHandlerIdHandlerMutexT> handlersLockMutexes;

                        decltype(MultiHandlerBase::m_handlerNodes) handlerNodesToDisconnect;
                        if (typename BusType::Context* context = BusType::GetContext())
                        {
                            AZStd::scoped_lock<decltype(context->m_contextMutex)> contextLock(context->m_contextMutex);
                            handlerNodesToDisconnect = AZStd::move(MultiHandlerBase::m_handlerNodes);

                            // One mutex per handler, reserve space for them
                            handlersLockMutexes.reserve(handlerNodesToDisconnect.size());

                            for (const auto& nodePair : handlerNodesToDisconnect)
                            {
                                HandlerNode* handlerNode = nodePair.second;

                                // Store the id handler mutex, since the handler node is going to be deleted
                                if (handlerNode->m_holder)
                                {
                                    handlersLockMutexes.push_back(handlerNode->m_holder->m_handlerMutex);
                                }

                                BusType::DisconnectInternal(*context, *handlerNode);

                                handlerNode->~HandlerNode();
                                handlerNodesToDisconnect.get_allocator().deallocate(handlerNode, sizeof(HandlerNode), AZStd::alignment_of<HandlerNode>::value);
                            }
                        }

                        // Lock and unlock immediately the ID handler mutex; if inside dispatch, this prevents the destruction
                        // of the handler object until dispatch completes.
                        // Note that in the multihandler case, this handler object might be attached to multiple IDs and so have
                        // multiple potential dispatches currently active.
                        AZStd::lock_range(handlersLockMutexes.begin(), handlersLockMutexes.end());
                        for (auto& m : handlersLockMutexes)
                        {
                            m.unlock();
                        }
                    }
                };
            };

            // Extends the default ebus implementation with a custom dispatched
            template<class Interface>
            class AssetBusImpl : public AZ::EBus<Interface>
            {
                using Base = AZ::EBus<Interface>;
                using HandlerBase = typename Base::Handler;
                using MultiHandlerBase = typename Base::MultiHandler;
                using IdType = typename Base::BusIdType;

            public:
                template <typename Function, typename... ArgsT>
                static void Event(const IdType& id, Function&& func, ArgsT&&... args)
                {
                    using Bus = Base;
                    using Traits = typename Bus::Traits;
                    using StoragePolicy = typename Bus::StoragePolicy;

                    if (auto* context = Bus::GetContext())
                    {
                        // The handlers which will recieve this event and the lock which protects these handlers from deletion
                        // Note that the move constructor/operator of AZStd::unique_lock are broken!
                        AZStd::vector<Interface*> handlersToSignal;
                        AssetBusHandlerIdHandlerMutexT handlersMutex;
                        std::unique_lock<AZStd::recursive_mutex> handlersLock;

                        // Gather handler and its lock while holding the context lock
                        {
                            typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                            EBUS_DO_ROUTING(*context, &id, false, false);

                            auto& addresses = context->m_buses.m_addresses;
                            auto addressIt = addresses.find(id);
                            if (addressIt != addresses.end())
                            {
                                auto& holder = *addressIt;
                                holder.add_ref();

                                // Save the lock we should hold
                                handlersMutex = holder.m_handlerMutex;
                                handlersLock = std::unique_lock<AZStd::recursive_mutex>(handlersMutex.get_mutex(), std::defer_lock);

                                auto& handlers = holder.m_handlers;
                                auto handlerIt = handlers.begin();
                                auto handlersEnd = handlers.end();

                                while (handlerIt != handlersEnd)
                                {
                                    auto itr = handlerIt++;
                                    handlersToSignal.push_back(*itr);
                                }

                                holder.release();
                            }

                            // Acquire the lock (if it exists), while holding the context lock
                            if (handlersLock.mutex() && !handlersToSignal.empty())
                            {
                                handlersLock.lock();
                            }
                            else
                            {
                                // this means there are no handlers for this ID, early exit
                                return;
                            }
                        }

                        // note: we now should be holding the lock
                        AZ_Assert(handlersLock.owns_lock(), AZ_FUNCTION_SIGNATURE " - precondition failed, handlers mutex should be now locked");

                        // Signal all gathered handlers
                        // Note that unlike normal EBuses, there is no need for the disconnect fixer
                        // because we don't iterate over the handler map while it might be modified
                        for (Interface* h : handlersToSignal)
                        {
                            Traits::EventProcessingPolicy::Call(func, h, args...);
                        }
                    }
                }
            };
        }
    }
}
