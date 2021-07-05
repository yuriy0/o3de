/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <IGem.h>
#include <IConsole.h>
#include <cstdint>
#include <cstdio> // snprintf
#include <limits>
#include <AzCore/StringFunc/StringFunc.h>

#include "ProfileTelemetryComponent.h"

namespace RADTelemetry
{
#ifdef AZ_PROFILE_TELEMETRY
    using TelemetryRequestBus = RADTelemetry::ProfileTelemetryRequestBus;
    using TelemetryRequests = RADTelemetry::ProfileTelemetryRequests;
    using MaskType = AZ::Debug::ProfileCategoryPrimitiveType;

    static const char* s_telemetryAddress;
    static int s_telemetryPort;
    static const char* s_telemetryCaptureMask;
    static int s_memCaptureEnabled;
    static int s_frameAdvanceType;

    using FrameAdvanceType = AZ::Debug::ProfileFrameAdvanceType;

    static void MaskCvarChangedCallback(ICVar*)
    {
        if (!s_telemetryCaptureMask || !s_telemetryCaptureMask[0])
        {
            return;
        }

        // Parse as a 64-bit hex string
        char* endPtr;
        MaskType maskCvarValue = strtoull(s_telemetryCaptureMask, &endPtr, 16);
        if (endPtr == s_telemetryCaptureMask)
        {
            MaskType defaultMask = 0;
            TelemetryRequestBus::BroadcastResult(defaultMask, &TelemetryRequests::GetDefaultCaptureMask);

            AZ_Error("RADTelemetryGem", false, "Invalid RAD Telemetry capture mask cvar value: %s, using default capture mask 0x%" PRIx64, s_telemetryCaptureMask, defaultMask);
            maskCvarValue = defaultMask;
        }

        // Mask off the memory capture flag and add it back if memory capture is enabled
        const MaskType fullCaptureMask = (maskCvarValue & ~AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(AZ::Debug::ProfileCategory::MemoryReserved)) | (s_memCaptureEnabled ? AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(AZ::Debug::ProfileCategory::MemoryReserved) : 0);
        TelemetryRequestBus::Broadcast(&TelemetryRequests::SetCaptureMask, fullCaptureMask);
    }

    static void FrameAdvancedTypeCvarChangedCallback(ICVar*)
    {
        TelemetryRequestBus::Broadcast(&TelemetryRequests::SetFrameAdvanceType, (s_frameAdvanceType == 0) ? FrameAdvanceType::Game : FrameAdvanceType::Render);
    }

    static void CmdTelemetryToggleEnabled([[maybe_unused]] IConsoleCmdArgs* args)
    {
        TelemetryRequestBus::Broadcast(&TelemetryRequests::SetAddress, s_telemetryAddress, s_telemetryPort);

        FrameAdvancedTypeCvarChangedCallback(nullptr); // Set frame advance type
        MaskCvarChangedCallback(nullptr); // Set the capture mask

        TelemetryRequestBus::Broadcast(&TelemetryRequests::ToggleEnabled);
    }
#endif

    class RADTelemetryModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(RADTelemetryModule, "{50BB63A6-4669-41F2-B93D-6EB8529413CD}", CryHooksModule);

        RADTelemetryModule()
            : CryHooksModule()
        {
#ifdef AZ_PROFILE_TELEMETRY
            m_descriptors.insert(m_descriptors.end(), {
                ProfileTelemetryComponent::CreateDescriptor(),
            });
#endif
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            AZ::ComponentTypeList components;

#ifdef AZ_PROFILE_TELEMETRY
            components.insert(components.end(),
                azrtti_typeid<ProfileTelemetryComponent>()
            );
#endif

            return components;
        }

        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& initParams) override
        {
            CryHooksModule::OnCrySystemInitialized(system, initParams);
            
#ifdef AZ_PROFILE_TELEMETRY
            REGISTER_COMMAND("radtm_ToggleEnabled", &CmdTelemetryToggleEnabled, 0, "Enabled or Disable RAD Telemetry");

            REGISTER_CVAR2("radtm_Address", &s_telemetryAddress, "127.0.0.1", VF_NULL, "The IP address for the telemetry server");
            REGISTER_CVAR2("radtm_Port", &s_telemetryPort, 4719, VF_NULL, "The port for the RAD telemetry server");
            REGISTER_CVAR2("radtm_MemoryCaptureEnabled", &s_memCaptureEnabled, 0, VF_NULL, "Toggle for telemetry memory capture");

            const int defaultFrameAdvanceTypeCvarValue = (FrameAdvanceType::Default == FrameAdvanceType::Game) ? 0 : 1;
            REGISTER_CVAR2_CB("radtm_FrameAdvanceType", &s_frameAdvanceType, defaultFrameAdvanceTypeCvarValue, VF_NULL, "Advance profile frames from either: =0 the main thread, or =1 render frame advance", FrameAdvancedTypeCvarChangedCallback);

            // Get the default value from ProfileTelemetryComponent
            MaskType defaultCaptureMaskValue = 0;
            TelemetryRequestBus::BroadcastResult(defaultCaptureMaskValue, &TelemetryRequests::GetCaptureMask);

            char defaultCaptureMaskStr[19];
            azsnprintf(defaultCaptureMaskStr, AZ_ARRAY_SIZE(defaultCaptureMaskStr), "0x%" PRIx64, defaultCaptureMaskValue);
            REGISTER_CVAR2_CB("radtm_CaptureMask", &s_telemetryCaptureMask, defaultCaptureMaskStr, VF_NULL, "A hex bitmask for the categories to be captured, 0x0 for all", MaskCvarChangedCallback);

            system.GetIConsole()->AddCommand("radtm_ToggleCaptureMaskFlags",
            [](IConsoleCmdArgs* args) {
                auto radBus = RADTelemetry::ProfileTelemetryRequestBus::FindFirstHandler();
                if (!radBus) return;
                if (args->GetArgCount() != 3)
                {
                    AZ_Warning("RADTelemetryGem", false, "radtm_ToggleCaptureMaskFlags: expected 2 args, got %d", args->GetArgCount()-1);
                    return;
                }

                char* end;
                bool setOrUnset = (bool)::strtol(args->GetArg(2), &end, 10);
                if (end == args->GetArg(1))
                {
                    AZ_Warning("RADTelemetryGem", false, "radtm_ToggleCaptureMaskFlags: expected 2nd arg to be a number");
                    return;
                }

                AZStd::vector<AZStd::string> flags;
                AZ::StringFunc::Tokenize(args->GetArg(1), flags, ',');

                static const auto namesBegin = AZ::Debug::ProfileCategoryNames;
                static const auto namesEnd = namesBegin+AZ_ARRAY_SIZE(AZ::Debug::ProfileCategoryNames);

                AZ::Debug::ProfileCategoryPrimitiveType flagsMask = 0;
                for (const auto& flagName : flags)
                {
                    auto nameIt = AZStd::find_if(namesBegin, namesEnd, [&](const char* name)
                    {
                        return AZ::StringFunc::Equal(name, flagName.c_str(), false);
                    });
                    if (nameIt != namesEnd)
                    {
                        flagsMask |= AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(nameIt - namesBegin);
                    }
                    else
                    {
                        AZ_Warning("RADTelemetryGem", false, "radtm_ToggleCaptureMaskFlags: '%s' is not a valid profiling category", flagName.c_str());
                    }
                }

                AZ::Debug::ProfileCategoryPrimitiveType captureMask = radBus->GetCaptureMask();
                if (setOrUnset)
                {
                    captureMask |= flagsMask;
                }
                else
                {
                    captureMask &= ~flagsMask; 
                }
                radBus->SetCaptureMask(captureMask);

                // NB: must set the string cvar, we'll read back the string later
                gEnv->pConsole->ExecuteString(AZStd::string::format("radtm_CaptureMask %p", (void*)(captureMask)).c_str());
            }, 0, "Sets or unsets flags for particular categories for RAD Telemetry profiling.\nUsage: radtm_ToggleCaptureMaskFlags Flag0,Flag1,Flag2 [0/1]");
#endif
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_RADTelemetry, RADTelemetry::RADTelemetryModule)
