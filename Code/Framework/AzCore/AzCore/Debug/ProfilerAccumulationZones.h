#pragma once

#ifdef AZ_PROFILE_TELEMETRY
#include <RADTelemetry/ProfileTelemetryAccumulationZones.h>
#else
#define AZ_PROFILE_DECLARE_ACCUMULATION_ZONE(zoneName)
#define AZ_PROFILE_EMIT_ACCUMULATION_ZONE(category, zoneIdentifier, numIterations, ...)
#define AZ_PROFILE_ACCUMULATION_ZONE(category, zoneIdentifier)
#define AZ_PROFILE_ENTER_ACCUMULATION_ZONE(category, zoneIdentifier)
#define AZ_PROFILE_LEAVE_ACCUMULATION_ZONE(category, zoneIdentifier)
#endif