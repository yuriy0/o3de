#pragma once

#ifdef AZ_PROFILE_TELEMETRY
#include <RADTelemetry/ProfileTelemetryLocks.h>
#else
#define AZ_PROFILE_AUTO_LOCK_WRAPPER(_auto_lock, category, lock, ...) _auto_lock(lock)
#define AZ_PROFILE_TRY_LOCK_WRAPPER(_try_lock, category, lock, ...) _try_lock(lock)
#define AZ_PROFILE_UNLOCK_WRAPPER(_unlock, category, lock) _unlock(lock)
#endif