#pragma once

#include <RADTelemetry/ProfileTelemetry.h>

#ifdef AZ_PROFILE_TELEMETRY

struct tmLockHolder_impl
{
    tmLockHolder_impl(
        tm_uint64 capture_mask,
        tm_uint32 flags,
        const void* lockid,
        const char* file,
        tm_uint32 line,
        const char* name
    )
    : m_cap_mask(capture_mask)
    , m_lockid(lockid)
    , m_file(file)
    , m_line(line)
    {
        tmAcquiredLockEx(m_cap_mask, flags, m_lockid, m_file, m_line, name);
    }
    ~tmLockHolder_impl()
    {
        tmReleasedLockEx(m_cap_mask, m_lockid, m_file, m_line);
    }

    tmLockHolder_impl(const tmLockHolder_impl&) = delete;
    tmLockHolder_impl& operator=(const tmLockHolder_impl&) = delete;
    tmLockHolder_impl(tmLockHolder_impl&&) = delete;
    tmLockHolder_impl& operator=(tmLockHolder_impl&&) = delete;

    tm_uint64 m_cap_mask;
    const void* m_lockid;
    const char* m_file;
    tm_uint32 m_line;
};

#define tmLockHolderBase(_mask, _flags, _lockid, _name)\
    tmLockHolder_impl TM_UNIQUE_NAME(z)(_mask, _flags, _lockid, __FILE__, __LINE__, _name);

#define AZ_PROFILE_AUTO_LOCK_WRAPPER(_auto_lock, category, lock, ...)\
    tmStartWaitForLock(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), 0, &lock, __VA_ARGS__);\
    _auto_lock(lock);\
    tmEndWaitForLock(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category));\
    tmLockHolderBase(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), 0, &lock, __VA_ARGS__);

#define AZ_PROFILE_TRY_LOCK_WRAPPER(_try_lock, category, lock, ...)\
    ([&](){\
        tmStartWaitForLock(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), 0, &lock, __VA_ARGS__);\
        auto res = _try_lock(lock);\
        tmEndWaitForLock(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category));\
        if (res)\
        {\
            tmAcquiredLock(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), 0, &lock, __VA_ARGS__);\
        }\
        return res;\
    })()

#define AZ_PROFILE_UNLOCK_WRAPPER(_unlock, category, lock)\
    _unlock(lock);\
    tmReleasedLock(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), &lock);

#endif