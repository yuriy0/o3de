/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#define AZ_LOADSCREENCOMPONENT_ENABLED      (1)

#if AZ_LOADSCREENCOMPONENT_ENABLED

#include <AzCore/Component/ComponentBus.h>

class LoadScreenInterface
    : public AZ::ComponentBus
{
public:
    using MutexType = AZStd::recursive_mutex;

    //! Invoked when the load screen should be updated and rendered. Single threaded loading only.
    virtual void UpdateAndRender() = 0;

    //! Invoked when the game load screen should become visible.
    virtual void GameStart() = 0;

    //! Invoked when the level load screen should become visible.
    virtual void LevelStart() = 0;

    //! Invoked when the load screen should be paused.
    virtual void Pause() = 0;

    //! Invoked when the load screen should be resumed.
    virtual void Resume() = 0;

    //! Invoked when the load screen should be stopped.
    virtual void Stop() = 0;

    //! Invoked to find out if loading screen is playing.
    virtual bool IsPlaying() = 0;
};
using LoadScreenBus = AZ::EBus<LoadScreenInterface>;

//! Interface for notifying load screen providers that specific load events are happening.
//! This is meant to notify systems to connect/disconnect to the LoadScreenUpdateNotificationBus if necessary.
struct LoadScreenNotifications
    : public AZ::EBusTraits
{
    //! Invoked when the game/engine loading starts. Returns true if any provider handles this.
    virtual bool NotifyGameLoadStart(bool usingLoadingThread) = 0;

    //! Invoked when level loading starts. Returns true if any provider handles this.
    virtual bool NotifyLevelLoadStart(bool usingLoadingThread) = 0;

    //! Invoked when loading finishes.
    virtual void NotifyLoadEnd() = 0;
};
using LoadScreenNotificationBus = AZ::EBus<LoadScreenNotifications>;

//! Interface for triggering load screen updates and renders. Has different methods for single threaded vs multi threaded.
//! This is a separate bus from the LoadScreenNotificationBus to avoid threading issues and to allow implementers to conditionally attach
//! from inside LoadScreenNotificationBus::NotifyGameLoadStart/NotifyLevelLoadStart
struct LoadScreenUpdateNotifications
    : public AZ::EBusTraits
{
    //! Invoked when the load screen should be updated and rendered. Single threaded loading only.
    virtual void UpdateAndRender(float deltaTimeInSeconds) = 0;

    //! Invoked when the load screen should be updated. Multi-threaded loading only.
    virtual void LoadThreadUpdate(float deltaTimeInSeconds) = 0;

    //! Invoked when the load screen should be updated. Multi-threaded loading only.
    virtual void LoadThreadRender() = 0;
};
using LoadScreenUpdateNotificationBus = AZ::EBus<LoadScreenUpdateNotifications>;

class LoadCompletionQueryInterface
    : public AZ::EBusTraits
{
public:
    using MutexType = AZStd::recursive_mutex;

    virtual ~LoadCompletionQueryInterface() {}

    //! Called to determine if level loading should finally complete: the loading screen should be closed and the level contents should begin rendering
    // @param texStreamingDone True iff texture streaming is completed
    // @param timeSinceLevelStart Time in seconds since level was first loaded from disk
    // @param framesSinceLevelStart Number of frames since level was first loaded from disk
    virtual bool ShouldCompleteLevelLoading(bool texStreamingDone, float timeSinceLevelStart, size_t framesSinceLevelStart) = 0;

    //! Called to determine if level precache should complete: streaming system is placed into asynchronous mode
    // @param timeSinceLevelLoadCompleted Time in seconds since level loading was completed (`ShouldCompleteLevelLoading` returned true)
    virtual bool ShouldCompletePrecaching(float timeSinceLevelLoadCompleted) = 0;
};
using LoadCompletionQueryBus = AZ::EBus<LoadCompletionQueryInterface>;

#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
