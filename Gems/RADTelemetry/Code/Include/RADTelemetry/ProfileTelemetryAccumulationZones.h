#pragma once

#include <RADTelemetry/ProfileTelemetry.h>

/*
Accumulation zones:

   void foo()
   {
        AZ_PROFILE_FUNCTION(...);

        static const int K = ...;

        AZ_PROFILE_DECLARE_ACCUMULATION_ZONE(myLoop1);
        AZ_PROFILE_DECLARE_ACCUMULATION_ZONE(myLoop2);

        for (int i = 0; i < K; ++i)
        {
            {
                AZ_PROFILE_ACCUMULATION_ZONE(SomeCategory, myLoop1);
                ... do work
            }
            
            {
                AZ_PROFILE_ACCUMULATION_ZONE(SomeCategory, myLoop2);
                ... do work
            }
        }

        AZ_PROFILE_EMIT_ACCUMULATION_ZONE(SomeCategory, myLoop1, K, "Some Label");
        AZ_PROFILE_EMIT_ACCUMULATION_ZONE(SomeCategory, myLoop2, K, "Another Label");
   }
*/

#ifdef AZ_PROFILE_TELEMETRY
struct tmAccumulationZone
{
    tm_uint64 capture_mask;
    tm_int64& accumulator;

    tmAccumulationZone(const tmAccumulationZone&) = delete;
    tmAccumulationZone& operator=(const tmAccumulationZone&) = delete;
    tmAccumulationZone(tmAccumulationZone&&) = delete;
    tmAccumulationZone& operator=(tmAccumulationZone&&) = delete;

    tmAccumulationZone(tm_uint64 capture_mask_, tm_int64& accumulator_)
        : capture_mask(capture_mask_)
        , accumulator(accumulator_)
    {
        tmEnterAccumulationZone(capture_mask, accumulator);
    }
    ~tmAccumulationZone()
    {
        tmLeaveAccumulationZone(capture_mask, accumulator);
    }
};

#define AZ_PROFILE_INTERNAL_MAKE_ZONE_IDENTIFIER(zoneIdentifier) zoneIdentifier##_tmAccumulationZoneIdentifier
#define AZ_PROFILE_INTERNAL_MAKE_ZONE_IDENTIFIER_START_TIME(zoneIdentifier) zoneIdentifier##tmAccumulationZoneStartTime

#define AZ_PROFILE_DECLARE_ACCUMULATION_ZONE(zoneIdentifier)\
    tm_int64 AZ_PROFILE_INTERNAL_MAKE_ZONE_IDENTIFIER(zoneIdentifier) = 0;

#define AZ_PROFILE_EMIT_ACCUMULATION_ZONE(category, zoneIdentifier, numIterations, ...)\
    AZ_INTERNAL_PROF_VERIFY_CAT(category);\
    tm_uint64 AZ_PROFILE_INTERNAL_MAKE_ZONE_IDENTIFIER_START_TIME(zoneIdentifier) = 0;\
    tmEmitAccumulationZone(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), TMZF_NONE, & AZ_PROFILE_INTERNAL_MAKE_ZONE_IDENTIFIER_START_TIME(zoneIdentifier), numIterations, AZ_PROFILE_INTERNAL_MAKE_ZONE_IDENTIFIER(zoneIdentifier), __VA_ARGS__);

#define AZ_PROFILE_ACCUMULATION_ZONE(category, zoneIdentifier)\
    AZ_INTERNAL_PROF_VERIFY_CAT(category);\
    tmAccumulationZone TM_UNIQUE_NAME(accum_zone) (AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), AZ_PROFILE_INTERNAL_MAKE_ZONE_IDENTIFIER(zoneIdentifier));

#define AZ_PROFILE_ENTER_ACCUMULATION_ZONE(category, zoneIdentifier)\
    AZ_INTERNAL_PROF_VERIFY_CAT(category);\
    tmEnterAccumulationZone(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), AZ_PROFILE_INTERNAL_MAKE_ZONE_IDENTIFIER(zoneIdentifier));

#define AZ_PROFILE_LEAVE_ACCUMULATION_ZONE(category, zoneIdentifier)\
    AZ_INTERNAL_PROF_VERIFY_CAT(category);\
    tmLeaveAccumulationZone(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), AZ_PROFILE_INTERNAL_MAKE_ZONE_IDENTIFIER(zoneIdentifier));

#endif