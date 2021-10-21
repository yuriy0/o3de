/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTriggerAction.h>
#include <EMotionFX/Source/ObjectAffectedByParameterChanges.h>

#include <AzFramework/UI/ClassSelectionParameters.h>

namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class ValueParameter;

    /**
     * AnimGraphParameterAction is a specific type of trigger action that modifies the parameter.
     */
    class EMFX_API AnimGraphParameterAction
        : public AnimGraphTriggerAction
        , public ObjectAffectedByParameterChanges
    {
    public:
        AZ_RTTI(AnimGraphParameterAction, "{57329F53-3E8F-47FA-997D-FEF390CB2E57}", AnimGraphTriggerAction, ObjectAffectedByParameterChanges)
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphParameterAction();
        AnimGraphParameterAction(AnimGraph* animGraph);
        ~AnimGraphParameterAction();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        void TriggerAction(AnimGraphInstance* animGraphInstance) const override;

        AZ::Outcome<size_t> GetParameterIndex() const;
        void SetParameterName(const AZStd::string& parameterName);
        const AZStd::string& GetParameterName() const;

        void SetParameterValue(const AZStd::any& parameterValue);
        const AZStd::any& GetParameterValue() const;
        AZ::TypeId GetParameterType() const;

        void SetTriggerValue(float value) { m_parameterValue = value; }
        float GetTriggerValue() const {
            float val;
            return AZStd::any_numeric_cast<float>(&m_parameterValue, val) ? val : 0.f;
        }

        // ObjectAffectedByParameterChanges overrides
        void ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName) override;
        void ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange) override;
        void ParameterRemoved(const AZStd::string& oldParameterName) override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string GetParameterValueString() const;
        AzFramework::ClassSelectionParameters GetParameterValueSelection();
        bool GetParameterValueVisibility();
        AZStd::vector<AZ::TypeId> TypesValidForAttribute();
        AZStd::unordered_set<AZ::TypeId> TypesValidForAttribute_set();


        // Reflected data
        AZStd::string                            m_parameterName;
        AZStd::any                               m_parameterValue;

        // Runtime data
        AZ::Outcome<size_t>                      m_parameterIndex;
        const ValueParameter*                    m_valueParameter;
    };
} // namespace EMotionFX
