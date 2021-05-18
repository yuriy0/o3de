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
*/

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>

namespace AZ
{
    namespace Render
    {
        class DeferredFogGlobalNotifications
            : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::recursive_mutex;

            virtual ~DeferredFogGlobalNotifications() = default;

            virtual void OnDeferredFogGlobalSettingsChanged() {};
        };
        using DeferredFogGlobalNotificationBus = AZ::EBus<DeferredFogGlobalNotifications>;

        class DeferredFogSettingsInterface
        {
        public:
            AZ_RTTI(AZ::Render::DeferredFogSettingsInterface, "{4C760B7D-DCBB-4244-94E4-E17180CA722A}");

            inline void OnSettingsChanged()
            {
                DeferredFogGlobalNotificationBus::Broadcast(&DeferredFogGlobalNotifications::OnDeferredFogGlobalSettingsChanged);
            };

            virtual void SetEnabled(bool value) = 0;
            virtual bool GetEnabled() const = 0;

            virtual void SetUseNoiseTextureShaderOption(bool value)= 0;
            virtual bool GetUseNoiseTextureShaderOption() const = 0;
            virtual void SetEnableFogLayerShaderOption(bool value)= 0;
            virtual bool GetEnableFogLayerShaderOption() const = 0;

            // Auto-gen virtual getter and setter functions...
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        virtual ValueType Get##Name() const = 0;                                                        \
        virtual void Set##Name(ValueType val) = 0;                                                      \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        };

    }
}
