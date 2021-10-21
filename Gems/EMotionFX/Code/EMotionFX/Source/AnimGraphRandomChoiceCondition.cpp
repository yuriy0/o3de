/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraphRandomChoiceCondition.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/Random.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphRandomChoiceCondition, AnimGraphConditionAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphRandomChoiceCondition::UniqueData, AnimGraphObjectUniqueDataAllocator, 0);

    AnimGraphRandomChoiceCondition::AnimGraphRandomChoiceCondition()
        : AnimGraphTransitionCondition()
    {
    }


    AnimGraphRandomChoiceCondition::AnimGraphRandomChoiceCondition(AnimGraph* animGraph)
        : AnimGraphRandomChoiceCondition()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphRandomChoiceCondition::~AnimGraphRandomChoiceCondition()
    {
    }


    float AnimGraphRandomChoiceCondition::GetRandomChoiceWeight() const
    {
        return m_randomChoiceWeight;
    }

    AZStd::optional<AZStd::pair<AZStd::vector<AnimGraphRandomChoiceCondition*>, size_t>>
        AnimGraphRandomChoiceCondition::GetRandomChoiceConditionsOnSourceTransition(AnimGraphInstance*) const
    {
        auto thisTransition = GetTransition();
        if (!thisTransition) { return {}; }

        auto transitionSource = thisTransition->GetSourceNode();
        if (!transitionSource) { return {}; }

        auto parentNode = transitionSource->GetParentNode();
        if (!parentNode) { return {}; }

        auto parentStateMachine = azdynamic_cast<AnimGraphStateMachine*>(parentNode);
        if (!parentStateMachine)
        {
            AZ_Assert(false, "Internal error, transition parent node is not a state machine?");
            return {};
        }

        AZStd::vector<AnimGraphRandomChoiceCondition*> randomChoiceConditions;
        AZStd::optional<size_t> thisIx;

        const size_t numTransitions = parentStateMachine->GetNumTransitions();
        for (size_t ixTransition = 0; ixTransition < numTransitions; ++ixTransition)
        {
            auto stateMachineTransition = parentStateMachine->GetTransition(ixTransition);
            const size_t numConditions = stateMachineTransition->GetNumConditions();
            for (size_t ixCondition = 0; ixCondition < numConditions; ++ixCondition)
            {
                auto condition = stateMachineTransition->GetCondition(ixCondition);
                if (auto randomChoiceCondition = azdynamic_cast<AnimGraphRandomChoiceCondition*>(condition))
                {
                    randomChoiceConditions.push_back(randomChoiceCondition);

                    if (randomChoiceCondition == this)
                    {
                        thisIx = randomChoiceConditions.size() - 1;
                    }
                }
            }
        }

        if (!thisIx)
        {
            AZ_Assert(false, "Internal error - found random choice conditions but `this' was not one of those");
            return {};
        }

        return { { AZStd::move(randomChoiceConditions), *thisIx } };
    }


    bool AnimGraphRandomChoiceCondition::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTransitionCondition::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // get the palette name
    const char* AnimGraphRandomChoiceCondition::GetPaletteName() const
    {
        return "Random Choice Condition";
    }


    // reset the condition
    void AnimGraphRandomChoiceCondition::Reset(AnimGraphInstance* animGraphInstance)
    {
        auto mbChoices = GetRandomChoiceConditionsOnSourceTransition(animGraphInstance);
        if (!mbChoices)
        {
            AZ_Error("AnimGraphRandomChoiceCondition", false, "Internal error, can't get shared conditions on transitions");
            return;
        }
        const auto& [choices, thisIndex] = *mbChoices;

        // find the unique data and reset it
        animGraphInstance->FindOrCreateUniqueObjectData(this);

        // Update the state of each random choice transition, if we are the final condition in the list
        if (thisIndex == choices.size() - 1)
        {
            // Compute total weight
            const float totalWeight = std::accumulate(
                choices.begin(), choices.end(), 0.f, [](float res, auto* c0) { return c0->GetRandomChoiceWeight() + res; }
            );

            // Get a random value
            const float randomValue = animGraphInstance->GetLcgRandom().GetRandomFloat() * totalWeight;

            // Compute selected index
            size_t index = choices.size() - 1;
            float currentChoiceCdf = 0.f;
            for (auto choiceIt = choices.begin(); choiceIt != choices.end(); ++choiceIt)
            {
                auto* choice = *choiceIt;
                currentChoiceCdf += choice->GetRandomChoiceWeight();
                if (randomValue < currentChoiceCdf)
                {
                    index = choiceIt - choices.begin();
                    break;
                }
            }

            // Mark all choices
            for (auto choiceIt = choices.begin(); choiceIt != choices.end(); ++choiceIt)
            {
                const size_t choiceIndex = choiceIt - choices.begin();
                UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(*choiceIt));
                uniqueData->m_selected = choiceIndex == index;
            }
        }
    }


    // test the condition
    bool AnimGraphRandomChoiceCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // add the unique data for the condition to the anim graph
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));

        return uniqueData->m_selected;
    }

    // construct and output the information summary string for this object
    void AnimGraphRandomChoiceCondition::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format(
            "%s: Weight=%.2f", RTTI_GetTypeName(),
            m_randomChoiceWeight);
    }


    // construct and output the tooltip for this object
    void AnimGraphRandomChoiceCondition::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // add the condition type
        columnName = "Condition Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"165\"><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // add th weight
        columnName = "Weight: ";
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td>%.2f</td>", columnName.c_str(), m_randomChoiceWeight);
    }

    //--------------------------------------------------------------------------------
    // class AnimGraphRandomChoiceCondition::UniqueData
    //--------------------------------------------------------------------------------

    // constructor
    AnimGraphRandomChoiceCondition::UniqueData::UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
        : AnimGraphObjectData(object, animGraphInstance)
    {
    }


    // destructor
    AnimGraphRandomChoiceCondition::UniqueData::~UniqueData()
    {
    }


    void AnimGraphRandomChoiceCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphRandomChoiceCondition, AnimGraphTransitionCondition>()
            ->Version(1)
            ->Field("randomChoiceWeight", &AnimGraphRandomChoiceCondition::m_randomChoiceWeight)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphRandomChoiceCondition>("Random Choice Condition", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox,
                &AnimGraphRandomChoiceCondition::m_randomChoiceWeight, "Random weight", "Relative to other random choice conditions, the weight which determines the probability of selecting this condition")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
            ;
    }
} // namespace EMotionFX
