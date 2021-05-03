#pragma once

#include <AzCore/Memory/Memory.h>
#include <SceneAPIExt/Rules/IMotionZeroRule.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class MotionZeroRule
                : public IMotionZeroRule
            {
            public:
                AZ_RTTI(MotionZeroRule, "{79D0E407-B65D-4F60-A199-00188249C008}", IMotionZeroRule);
                AZ_CLASS_ALLOCATOR_DECL;

                MotionZeroRule();
                ~MotionZeroRule() override = default;

                bool DoZeroTranslation() const override;
                bool DoZeroRotation() const override;
                bool DoZeroScale() const override;

                static void Reflect(AZ::ReflectContext* context);

            protected:
                bool m_zeroTranslation;
                bool m_zeroRotation;
                bool m_zeroScale;
            };
        } // Rule
    } // Pipeline
} // EMotionFX