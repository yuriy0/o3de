/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    class EMFX_API AnimGraphRandomChoiceCondition
        : public AnimGraphTransitionCondition
    {
    public:
        AZ_RTTI(AnimGraphRandomChoiceCondition, "{F4FED145-588B-4C4D-84A1-F7DF52553F82}", AnimGraphTransitionCondition);
        AZ_CLASS_ALLOCATOR_DECL;

        // the unique data
        class EMFX_API UniqueData
            : public AnimGraphObjectData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

        public:
            AZ_CLASS_ALLOCATOR_DECL;

            UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance);
            ~UniqueData() override;

            bool m_selected = false;
        };

        AnimGraphRandomChoiceCondition();
        AnimGraphRandomChoiceCondition(AnimGraph* animGraph);
        ~AnimGraphRandomChoiceCondition();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        //void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;
        void Reset(AnimGraphInstance* animGraphInstance) override;
        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }

        static void Reflect(AZ::ReflectContext* context);

    private:
        float GetRandomChoiceWeight() const;

        AZStd::optional<AZStd::pair<AZStd::vector<AnimGraphRandomChoiceCondition*>, size_t>> GetRandomChoiceConditionsOnSourceTransition(AnimGraphInstance* animGraphInstance) const;

        // Reflected data
        float m_randomChoiceWeight = 1.f;
    };
}   // namespace EMotionFX
