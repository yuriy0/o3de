/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Asset/AssetCommon.h>

namespace EMotionFX
{
    class AnimGraphInstance;
    class AnimGraphPlaySpeedModifier;
    using AnimGraphPlaySpeedModifierPtr = AZStd::shared_ptr<AnimGraphPlaySpeedModifier>;
}

namespace EMotionFX
{
    namespace Integration
    {
        /**
         * EmotionFX Anim Graph Component Request Bus
         * Used for making requests to the EMotionFX Anim Graph Components.
         */
        class AnimGraphComponentRequests
            : public AZ::ComponentBus
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            /// Retrieves the component's live graph instance.
            /// \return pointer to anim graph instance.
            virtual EMotionFX::AnimGraphInstance* GetAnimGraphInstance() { return nullptr; }

            /// Add a playspeed modifier which is inactive.
            /// \param modifier The play speed modifier to apply. Must be zero or positive.
            virtual AnimGraphPlaySpeedModifierPtr AddPlayspeedModifier(float modifier) = 0;

            /// Retrieve parameter index for a given parameter name.
            /// Retrieving the index and using it to set parameter values is more performant than setting by name.
            /// \param parameterName - name of parameter for which to retrieve the index.
            /// \return parameter index
            virtual AZ::u32 FindParameterIndex(const char* parameterName) = 0;

            /// Updates a anim graph property given a value whose dynamic type matches the property type
            /// \param parameterIndex - index of parameter to set
            /// \param value - value to set
            virtual void SetParameter(AZ::u32 parameterIndex, AZStd::any value) = 0;

            /// Retrieves a anim graph property as a dynamic value
            /// \param parameterIndex - index of parameter to set
            virtual AZStd::any GetParameter(AZ::u32 parameterIndex) = 0;

            /// Updates a anim graph property given a value whose dynamic type matches the property type
            /// \param parameterName - name of parameter to set
            /// \param value - value to set
            virtual void SetNamedParameter(const char* parameterName, AZStd::any value) = 0;

            /// Retrieves a anim graph property as a dynamic value
            /// \param parameterName - name of parameter to set
            virtual AZStd::any GetNamedParameter(const char* parameterName) = 0;

            /// Retrieve parameter name for a given parameter index.
            /// \param parameterName - index of parameter for which to retrieve the name.
            /// \return parameter name
            virtual const char* FindParameterName(AZ::u32 parameterIndex) = 0;

            /// Updates a anim graph property given a float value.
            /// \param parameterIndex - index of parameter to set
            /// \param value - value to set
            virtual void SetParameterFloat(AZ::u32 parameterIndex, float value) = 0;

            /// Updates a anim graph property given a boolean value.
            /// \param parameterIndex - index of parameter to set
            /// \param value - value to set
            virtual void SetParameterBool(AZ::u32 parameterIndex, bool value) = 0;

            /// Updates a anim graph property given a string value.
            /// \param parameterIndex - index of parameter to set
            /// \param value - value to set
            virtual void SetParameterString(AZ::u32 parameterIndex, const char* value) = 0;

            /// Updates a anim graph property given a Vector2 value.
            /// \param parameterIndex - index of parameter to set
            /// \param value - value to set
            virtual void SetParameterVector2(AZ::u32 parameterIndex, const AZ::Vector2& value) = 0;

            /// Updates a anim graph property given a Vector3 value.
            /// \param parameterIndex - index of parameter to set
            /// \param value - value to set
            virtual void SetParameterVector3(AZ::u32 parameterIndex, const AZ::Vector3& value) = 0;

            /// Updates a anim graph property given euler rotation values.
            /// \param parameterIndex - index of parameter to set
            /// \param value - value to set
            virtual void SetParameterRotationEuler(AZ::u32 parameterIndex, const AZ::Vector3& value) = 0;

            /// Updates a anim graph property given a quaternion value.
            /// \param parameterIndex - index of parameter to set
            /// \param value - value to set
            virtual void SetParameterRotation(AZ::u32 parameterIndex, const AZ::Quaternion& value) = 0;


            /// Updates a anim graph property given a float value.
            /// \param parameterName - name of parameter to set
            /// \param value
            virtual void SetNamedParameterFloat(const char* parameterName, float value) = 0;

            /// Updates a anim graph property given a boolean value.
            /// \param parameterName - name of parameter to set
            /// \param value
            virtual void SetNamedParameterBool(const char* parameterName, bool value) = 0;

            /// Updates a anim graph property given a string value.
            /// \param parameterName - name of parameter to set
            /// \param value
            virtual void SetNamedParameterString(const char* parameterName, const char* value) = 0;

            /// Updates a anim graph property given a Vector2 value.
            /// \param parameterName - name of parameter to set
            /// \param value
            virtual void SetNamedParameterVector2(const char* parameterName, const AZ::Vector2& value) = 0;

            /// Updates a anim graph property given a Vector3 value.
            /// \param parameterName - name of parameter to set
            /// \param value
            virtual void SetNamedParameterVector3(const char* parameterName, const AZ::Vector3& value) = 0;

            /// Updates a anim graph property given euler rotation values.
            /// \param parameterName - name of parameter to set
            /// \param value
            virtual void SetNamedParameterRotationEuler(const char* parameterName, const AZ::Vector3& value) = 0;

            /// Updates a anim graph property given a quaternion value.
            /// \param parameterName - name of parameter to set
            /// \param value
            virtual void SetNamedParameterRotation(const char* parameterName, const AZ::Quaternion& value) = 0;

            /// Enable or disable debug draw visualization inside the anim graph instance.
            virtual void SetVisualizeEnabled(bool enabled) = 0;

            /// Retrieves a anim graph property as a float value.
            /// \param parameterIndex - index of parameter to set
            virtual float GetParameterFloat(AZ::u32 parameterIndex) = 0;

            /// Retrieves a anim graph property as a boolean value.
            /// \param parameterIndex - index of parameter to set
            virtual bool GetParameterBool(AZ::u32 parameterIndex) = 0;

            /// Retrieves a anim graph property given a string value.
            /// \param parameterIndex - index of parameter to set
            virtual AZStd::string GetParameterString(AZ::u32 parameterIndex) = 0;

            /// Retrieves a anim graph property as a Vector2 value.
            /// \param parameterIndex - index of parameter to set
            virtual AZ::Vector2 GetParameterVector2(AZ::u32 parameterIndex) = 0;

            /// Retrieves a anim graph property as a Vector3 value.
            /// \param parameterIndex - index of parameter to set
            virtual AZ::Vector3 GetParameterVector3(AZ::u32 parameterIndex) = 0;

            /// Retrieves a anim graph property given as euler rotation values.
            /// \param parameterIndex - index of parameter to set
            virtual AZ::Vector3 GetParameterRotationEuler(AZ::u32 parameterIndex) = 0;

            /// Retrieves a anim graph property as a quaternion value.
            /// \param parameterIndex - index of parameter to set
            virtual AZ::Quaternion GetParameterRotation(AZ::u32 parameterIndex) = 0;

            /// Retrieves a anim graph property as a float value.
            /// \param parameterName - name of parameter to get
            virtual float GetNamedParameterFloat(const char* parameterName) = 0;

            /// Retrieves a anim graph property as a boolean value.
            /// \param parameterName - name of parameter to get
            virtual bool GetNamedParameterBool(const char* parameterName) = 0;

            /// Retrieves a anim graph property given a string value.
            /// \param parameterName - name of parameter to get
            virtual AZStd::string GetNamedParameterString(const char* parameterName) = 0;

            /// Retrieves a anim graph property as a Vector2 value.
            /// \param parameterName - name of parameter to get
            virtual AZ::Vector2 GetNamedParameterVector2(const char* parameterName) = 0;

            /// Retrieves a anim graph property as a Vector3 value.
            /// \param parameterName - name of parameter to get
            virtual AZ::Vector3 GetNamedParameterVector3(const char* parameterName) = 0;

            /// Retrieves a anim graph property given as euler rotation values.
            /// \param parameterName - name of parameter to get
            virtual AZ::Vector3 GetNamedParameterRotationEuler(const char* parameterName) = 0;

            /// Retrieves a anim graph property as a quaternion value.
            /// \param parameterName - name of parameter to get
            virtual AZ::Quaternion GetNamedParameterRotation(const char* parameterName) = 0;            

            /// Check whether debug visualization is enabled or not.
            virtual bool GetVisualizeEnabled() = 0;

            /// Making a request to sync the anim graph with another animg graph
            /// \param leaderEntityId - the entity id of another anim graph.
            virtual void SyncAnimGraph(AZ::EntityId leaderEntityId) = 0;

            /// Making a request to desync from the anim graph to its leader graph
            /// \param leaderEntityId - the entity id of another anim graph.
            virtual void DesyncAnimGraph(AZ::EntityId leaderEntityId) = 0;

            /// Set the current active motion set to the given child
            virtual void SetMotionSet(const AZStd::string& motionSetName) = 0;
        };

        using AnimGraphComponentRequestBus = AZ::EBus<AnimGraphComponentRequests>;

		class AnimGraphNodeRequests
			: public AZ::ComponentBus
		{
		public:
			virtual ~AnimGraphNodeRequests() = default;
			
			static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

			/// Retrieves a node Id by name given.
			/// \param parameterName - name of node to get
			virtual AZ::u64 GetNodeIdByName(const char* nodeName) = 0;

			/// Retrieves the duration of an animgraph node
			virtual float GetDuration(const char* nodeName) = 0;

			/// Retrieves the current play time of an animgraph node
			virtual float GetPlaytime(const char* nodeName) = 0;

			/// Retrieves the play speed of an animgraph node
			virtual float GetPlayspeed(const char* nodeName) = 0;

			/// Sets the current play time of an animgraph node
			virtual void SetPlayTime(const char* nodeName, float playtime) = 0;

			/// Sets the play speed of an animgraph node
			virtual void SetPlaySpeed(const char* nodeName, float speed) = 0;

			/// Rewind the anim graph node
			virtual void Rewind(const char* nodeName) = 0;
		};

		using AnimGraphNodeRequestBus = AZ::EBus<AnimGraphNodeRequests>;

        /**
         * EmotionFX Anim Graph Component Notification Bus
         * Used for monitoring events from Anim Graph components.
         */
        class AnimGraphComponentNotifications
            : public AZ::ComponentBus
        {
        public:

            //////////////////////////////////////////////////////////////////////////
            /**
             * Custom connection policy notifies connecting listeners immediately if anim graph instance is already created.
             */
            template<class Bus>
            struct AssetConnectionPolicy
                : public AZ::EBusConnectionPolicy<Bus>
            {
                static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
                {
                    AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                    EMotionFX::AnimGraphInstance* instance = nullptr;
                    AnimGraphComponentRequestBus::EventResult(instance, id, &AnimGraphComponentRequestBus::Events::GetAnimGraphInstance);
                    if (instance)
                    {
                        handler->OnAnimGraphInstanceCreated(instance);
                    }
                }
            };
            template<typename Bus>
            using ConnectionPolicy = AssetConnectionPolicy<Bus>;
            //////////////////////////////////////////////////////////////////////////

            /// Notifies listeners when the component has created a graph instance.
            /// \param animGraphInstance - pointer to anim graph instance
            virtual void OnAnimGraphInstanceCreated(EMotionFX::AnimGraphInstance* /*animGraphInstance*/) {};

            /// Notifies listeners when the component is destroying a graph instance.
            /// \param animGraphInstance - pointer to anim graph instance
            virtual void OnAnimGraphInstanceDestroyed(EMotionFX::AnimGraphInstance* /*animGraphInstance*/) {};

            /// Notifies listeners when any parameter changes
            /// \param animGraphInstance - pointer to anim graph instance
            /// \param parameterIndex - index of changed parameter
            /// \param beforeValue - value before the change
            /// \param afterValue - value after the change
            virtual void OnAnimGraphParameterChanged(EMotionFX::AnimGraphInstance* /*animGraphInstance*/, [[maybe_unused]] AZ::u32 parameterIndex, [[maybe_unused]] AZStd::any beforeValue, [[maybe_unused]] AZStd::any afterValue) {};

            /// Notifies listeners when a float parameter changes
            /// \param animGraphInstance - pointer to anim graph instance
            /// \param parameterIndex - index of changed parameter
            /// \param beforeValue - value before the change
            /// \param afterValue - value after the change
            virtual void OnAnimGraphFloatParameterChanged(EMotionFX::AnimGraphInstance* /*animGraphInstance*/, [[maybe_unused]] AZ::u32 parameterIndex, [[maybe_unused]] float beforeValue, [[maybe_unused]] float afterValue) {};

            /// Notifies listeners when a bool parameter changes
            /// \param animGraphInstance - pointer to anim graph instance
            /// \param parameterIndex - index of changed parameter
            /// \param beforeValue - value before the change
            /// \param afterValue - value after the change
            virtual void OnAnimGraphBoolParameterChanged(EMotionFX::AnimGraphInstance* /*animGraphInstance*/, [[maybe_unused]] AZ::u32 parameterIndex, [[maybe_unused]] bool beforeValue, [[maybe_unused]] bool afterValue) {};

            /// Notifies listeners when a string parameter changes
            /// \param animGraphInstance - pointer to anim graph instance
            /// \param parameterIndex - index of changed parameter
            /// \param beforeValue - value before the change
            /// \param afterValue - value after the change
            virtual void OnAnimGraphStringParameterChanged(EMotionFX::AnimGraphInstance* /*animGraphInstance*/, [[maybe_unused]] AZ::u32 parameterIndex, [[maybe_unused]] const char* beforeValue, [[maybe_unused]] const char* afterValue) {};

            /// Notifies listeners when a vector2 parameter changes
            /// \param animGraphInstance - pointer to anim graph instance
            /// \param parameterIndex - index of changed parameter
            /// \param beforeValue - value before the change
            /// \param afterValue - value after the change
            virtual void OnAnimGraphVector2ParameterChanged(EMotionFX::AnimGraphInstance* /*animGraphInstance*/, [[maybe_unused]] AZ::u32 parameterIndex, [[maybe_unused]] const AZ::Vector2& beforeValue, [[maybe_unused]] const AZ::Vector2& afterValue) {};

            /// Notifies listeners when a vector3 parameter changes
            /// \param animGraphInstance - pointer to anim graph instance
            /// \param parameterIndex - index of changed parameter
            /// \param beforeValue - value before the change
            /// \param afterValue - value after the change
            virtual void OnAnimGraphVector3ParameterChanged(EMotionFX::AnimGraphInstance* /*animGraphInstance*/, [[maybe_unused]] AZ::u32 parameterIndex, [[maybe_unused]] const AZ::Vector3& beforeValue, [[maybe_unused]] const AZ::Vector3& afterValue) {};

            /// Notifies listeners when a rotation parameter changes
            /// \param animGraphInstance - pointer to anim graph instance
            /// \param parameterIndex - index of changed parameter
            /// \param beforeValue - value before the change
            /// \param afterValue - value after the change
            virtual void OnAnimGraphRotationParameterChanged(EMotionFX::AnimGraphInstance* /*animGraphInstance*/, [[maybe_unused]] AZ::u32 parameterIndex, [[maybe_unused]] const AZ::Quaternion& beforeValue, [[maybe_unused]] const AZ::Quaternion& afterValue) {};

            /// Notifies listeners when an another anim graph trying to sync this graph
            /// \param animGraphInstance - pointer to the follower anim graph instance
            virtual void OnAnimGraphSynced(EMotionFX::AnimGraphInstance* /*animGraphInstance(Follower)*/) {};

            /// Notifies listeners when an another anim graph trying to desync this graph
            /// \param animGraphInstance - pointer to the follower anim graph instance
            virtual void OnAnimGraphDesynced(EMotionFX::AnimGraphInstance* /*animGraphInstance(Follower)*/) {};
        };

        using AnimGraphComponentNotificationBus = AZ::EBus<AnimGraphComponentNotifications>;


        //Editor
        class EditorAnimGraphComponentRequests
            : public AZ::ComponentBus
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            /// Retrieves the component's Animgraph AssetId.
            /// \return assetId
            virtual const AZ::Data::AssetId& GetAnimGraphAssetId() = 0;

            /// Retrieves the component's MotionSet AssetId.
            /// \return assetId
            virtual const AZ::Data::AssetId& GetMotionSetAssetId() = 0;
        };

        using EditorAnimGraphComponentRequestBus = AZ::EBus<EditorAnimGraphComponentRequests>;        
    } // namespace Integration
} // namespace EMotionFX
