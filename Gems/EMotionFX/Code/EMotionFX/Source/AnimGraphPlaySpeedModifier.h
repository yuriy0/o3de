#pragma once

#include "AnimGraphPlaySpeedModifier_fwd.h"

namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    /*
        Create an instance of this class through AnimGraphInstance::MakePlaySpeedModifier, then call these class functions to temporarily (for a duration)
        change the playspeed of the animgraph.
     */
    class AnimGraphPlaySpeedModifier
    {
    public:
        friend class AnimGraphInstance;

        // Add the specified duration to this modifier
        void Emplace(float duration);

        // Remove the specified duration from this modifier
        void Remove(float duration);

        // Remove all remaining duration
        void Stop();

        // Getters
        float GetRemainingDuration() const;
        float GetSpeedMultiplier() const;
    private:
        AZ_DISABLE_COPY_MOVE(AnimGraphPlaySpeedModifier);

        AnimGraphPlaySpeedModifier(AnimGraphInstance* owner, float speedMult);

        using ModifyFn = void(AnimGraphPlaySpeedModifier::*)(float&, float);

        void Modify_None(float& dt, float dtOrig);
        void Modify_Some(float& dt, float dtOrig);
        void Remove_Internal(float duration);

        ModifyFn           Modify;
        float              m_speedMult;
        float              m_remainingDuration;
        AnimGraphInstance* m_owner;
    };
}