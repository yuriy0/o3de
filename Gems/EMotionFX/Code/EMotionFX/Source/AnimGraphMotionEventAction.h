#pragma once

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTriggerAction.h>
#include <EMotionFX/Source/ObjectAffectedByParameterChanges.h>

#include <EMotionFX/Source/Event.h>
#include <EMotionFX/Source/MotionEvent.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class ValueParameter;

    /**
    * AnimGraphMotionEventAction is a specific type of trigger action that sends a motion event
    */
    class EMFX_API AnimGraphMotionEventAction
        : public AnimGraphTriggerAction
    {
    public:
        AZ_RTTI(AnimGraphMotionEventAction, "{01A25F81-ACFC-4746-A1A0-384C3D77339A}", AnimGraphTriggerAction);
        AZ_CLASS_ALLOCATOR_DECL;

        static void Reflect(AZ::ReflectContext* context);

        AnimGraphMotionEventAction();
        AnimGraphMotionEventAction(AnimGraph* animGraph);
        ~AnimGraphMotionEventAction();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        void TriggerAction(AnimGraphInstance* animGraphInstance) const override;

        //// ObjectAffectedByParameterChanges overrides
        //void ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName) override;
        //void ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange) override;
        //void ParameterRemoved(const AZStd::string& oldParameterName) override;

    private:
        // Reflected data
        Event m_event;

        // Runtime data
        MotionEvent m_wrappedEvent;
    };
} // namespace EMotionFX
