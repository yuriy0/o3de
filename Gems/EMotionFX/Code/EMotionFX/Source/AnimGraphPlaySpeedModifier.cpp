#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphPlaySpeedModifier.h>

namespace EMotionFX {
    ////////////////////////////////////////////////////
    // AnimGraphPlaySpeedModifier
    ////////////////////////////////////////////////////
    AnimGraphPlaySpeedModifierPtr AnimGraphInstance::AddPlayspeedModifier(float mod) {
        MCore::LockGuard _(mPlaySpeedModifiersMutex);

        // We create and return a shared pointer, but do NOT store a shared pointer
        // When the count reaches zero, the deleter removes it from this animgraph instance, and then deletes it
        AnimGraphPlaySpeedModifierPtr m =
            AnimGraphPlaySpeedModifierPtr(
                new AnimGraphPlaySpeedModifier(this, mod),
                [](AnimGraphPlaySpeedModifier* m) {
                    if (m->m_owner) { m->m_owner->RemovePlayspeedModifier(m); }
                    delete m;
        });
        mPlaySpeedModifiers.push_back(m.get());

        AZ_Assert(m.use_count() == 1, "AnimGraphInstance::AddPlayspeedModifier: Interal error - use count must be 1");

        return m;
    }

    void AnimGraphInstance::RemovePlayspeedModifier(AnimGraphPlaySpeedModifier* mod) {
        MCore::LockGuard _(mPlaySpeedModifiersMutex);

        auto it = AZStd::find(mPlaySpeedModifiers.begin(), mPlaySpeedModifiers.end(), mod);
        if (it != mPlaySpeedModifiers.end()) {
            mPlaySpeedModifiers.erase(it);
        } else {
            AZ_Assert(false, "AnimGraphInstance::RemovePlayspeedModifier: 'this' doesn't own the given pointer");
        }
    }

    AnimGraphPlaySpeedModifier::AnimGraphPlaySpeedModifier(AnimGraphInstance* owner, float speedMult)
        : m_owner(owner)
        , m_speedMult(AZ::GetMax(speedMult, 0.f))
        , m_remainingDuration(0.f)
        , Modify(&AnimGraphPlaySpeedModifier::Modify_None) {}

    void AnimGraphPlaySpeedModifier::Modify_None(float&, float) {}

    void AnimGraphPlaySpeedModifier::Modify_Some(float& dt, float dtOrig) {
        if (dtOrig < m_remainingDuration) {
            // Our duration has not yet elapsed after this tick
            dt *= m_speedMult;
            m_remainingDuration -= dtOrig;
        } else {
            // Our duration will elapse this tick.
            // Compute the proportion of the remaining duration relative to the current tick time. Then multiply the full-tick delta
            // by that proportion, and add that delta instead.

            auto proportion = m_remainingDuration/dtOrig;
            if (proportion <= FLT_EPSILON) {
                // Edge case; almost certainly does not occur
                dt *= m_speedMult;
                m_remainingDuration -= dtOrig;
                Modify = &AnimGraphPlaySpeedModifier::Modify_None;
            } else {
                const auto scaledDelta = proportion * (dt*m_speedMult - dt);
                dt += scaledDelta;

                m_remainingDuration = 0.f;
                Modify = &AnimGraphPlaySpeedModifier::Modify_None;
            }
        }
    }

    void AnimGraphPlaySpeedModifier::Emplace(float duration) {
        if (duration == 0.f) return;
        AZ_Assert(duration >= 0.f, __FUNCTION__ " - duration must be positive");
        duration = AZ::GetMax(duration, 0.f);

        m_remainingDuration += duration;
        Modify = &AnimGraphPlaySpeedModifier::Modify_Some;
    }

    void AnimGraphPlaySpeedModifier::Remove_Internal(float duration) {
        m_remainingDuration -= duration;
        if (m_remainingDuration <= 0.f) { Stop(); }
    }

    void AnimGraphPlaySpeedModifier::Remove(float duration) {
        if (duration == 0.f) return;
        AZ_Assert(duration >= 0.f, __FUNCTION__ " - duration must be positive");
        duration = AZ::GetMax(duration, 0.f);
        Remove_Internal(duration);
    }

    void AnimGraphPlaySpeedModifier::Stop() {
        m_remainingDuration = 0.f;
        Modify = &AnimGraphPlaySpeedModifier::Modify_None;
    }

    float AnimGraphPlaySpeedModifier::GetRemainingDuration() const {
        return m_remainingDuration;
    }

    float AnimGraphPlaySpeedModifier::GetSpeedMultiplier() const {
        return m_speedMult;
    }
}