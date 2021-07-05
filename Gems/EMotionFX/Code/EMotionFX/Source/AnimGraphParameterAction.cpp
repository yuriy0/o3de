/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphParameterAction.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeFloat.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphParameterAction, AnimGraphAllocator, 0)

    AnimGraphParameterAction::AnimGraphParameterAction()
        : AnimGraphTriggerAction()
        , m_parameterIndex(AZ::Failure())
        , m_valueParameter(nullptr)
    {
    }


    AnimGraphParameterAction::AnimGraphParameterAction(AnimGraph* animGraph)
        : AnimGraphParameterAction()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphParameterAction::~AnimGraphParameterAction()
    {

    }


    void AnimGraphParameterAction::Reinit()
    {
        // Find the parameter index for the given parameter name, to prevent string based lookups every frame
        m_parameterIndex = mAnimGraph->FindValueParameterIndexByName(m_parameterName);
        if (m_parameterIndex.IsSuccess())
        {
            m_valueParameter = mAnimGraph->FindValueParameter(m_parameterIndex.GetValue());
        }
        else
        {
            m_valueParameter = nullptr;
        }

        // If the value is no longer valid for the parameter type, reset it
        if (!m_parameterValue.empty()) {
            auto validTypes = TypesValidForAttribute_set();
            if (validTypes.find(m_parameterValue.type()) == validTypes.end()) {
                m_parameterValue = AZStd::any();
            }
        }
    }

    bool AnimGraphParameterAction::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTriggerAction::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* AnimGraphParameterAction::GetPaletteName() const
    {
        return "Parameter Action";
    }

    AzFramework::ClassSelectionParameters AnimGraphParameterAction::GetParameterValueSelection() {
        return AzFramework::ClassSelectionParameters(TypesValidForAttribute());
    }

    bool AnimGraphParameterAction::GetParameterValueVisibility() {
        // Only show the parameter value when a valid parameter name is selected
        return m_parameterIndex.IsSuccess();
    }

    AZStd::unordered_set<AZ::TypeId> AnimGraphParameterAction::TypesValidForAttribute_set() {
        AZStd::unordered_set<AZ::TypeId> res;

        if (!m_parameterIndex.IsSuccess()) { return res; }
        auto paramIndex = m_parameterIndex.GetValue();

        auto animGraph = GetAnimGraph();
        if (!animGraph) { return res; }

        auto param = animGraph->FindValueParameter(paramIndex);
        if (!param) { return res; }

        return param->GetAcceptedTypes();
    }

    AZStd::vector<AZ::TypeId> AnimGraphParameterAction::TypesValidForAttribute() {
        auto ts = TypesValidForAttribute_set();
        AZStd::vector<AZ::TypeId> res;
        res.insert(res.begin(), ts.begin(), ts.end());
        return res;
    }

    void AnimGraphParameterAction::TriggerAction(AnimGraphInstance* animGraphInstance) const
    {
        if (!m_parameterIndex.IsSuccess())
        {
            return;
        }
        
        const uint32 paramIndex = static_cast<uint32>(m_parameterIndex.GetValue());
        MCore::Attribute* attribute = animGraphInstance->GetParameterValue(paramIndex);
        if (!attribute)
        {
            return;
        }

        AZStd::any prevVal;
        if (!attribute->ToAny(prevVal)) {
            AZ_Error("EMotionFX", false, "Failed to get parameter '%s' as an AZStd::any value", m_parameterName.c_str());
        }

        if (attribute->FromAny(m_parameterValue)) {
            AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnParameterActionTriggered, m_valueParameter);
        } else {
            AZ_Error("EMotionFX", false, "Failed to set parameter '%s'", m_parameterName.c_str());
        }
    }

    AZ::Outcome<size_t> AnimGraphParameterAction::GetParameterIndex() const
    {
        return m_parameterIndex;
    }

    void AnimGraphParameterAction::SetParameterName(const AZStd::string& parameterName)
    {
        m_parameterName = parameterName;
        if (mAnimGraph)
        {
            Reinit();
        }
    }

    void AnimGraphParameterAction::SetParameterValue(const AZStd::any& parameterValue)
    {
        m_parameterValue = parameterValue;
        if (mAnimGraph)
        {
            Reinit();
        }
    }


    const AZStd::string& AnimGraphParameterAction::GetParameterName() const
    {
        return m_parameterName;
    }

    const AZStd::any& AnimGraphParameterAction::GetParameterValue() const
    {
        return m_parameterValue;
    }


    AZStd::string AnimGraphParameterAction::GetParameterValueString() const
    {
        if (m_parameterValue.empty()) { return "{}"; }

        AZStd::vector<char> buf; buf.reserve(256);
        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&buf);
        if (AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_JSON, AZStd::any_cast<void>(&m_parameterValue), m_parameterValue.type())) {
            return AZStd::string(buf.data());
        }
        return "<unknown>";
    }

    void AnimGraphParameterAction::GetSummary(AZStd::string * outResult) const
    {
        *outResult = AZStd::string::format(
            "%s: Parameter Name='%s', Parameter Value='%s'", 
            RTTI_GetTypeName(),
            m_parameterName.c_str(),
            GetParameterValueString().c_str()
        );
    }

    // construct and output the tooltip for this object
    void AnimGraphParameterAction::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // add the action type
        columnName = "Action Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"120\"><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

        // add the parameter
        columnName = "Parameter Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), m_parameterName.c_str());

        // add the value string
        columnName = "Parameter Value: ";
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), GetParameterValueString().c_str());
    }

    void AnimGraphParameterAction::ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName)
    {
        if (m_parameterName == oldParameterName)
        {
            SetParameterName(newParameterName);
        }
    }

    void AnimGraphParameterAction::ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange)
    {
        AZ_UNUSED(beforeChange);
        AZ_UNUSED(afterChange);
        m_parameterIndex = mAnimGraph->FindValueParameterIndexByName(m_parameterName);
    }

    void AnimGraphParameterAction::ParameterRemoved(const AZStd::string& oldParameterName)
    {
        if (oldParameterName == m_parameterName)
        {
            m_parameterName.clear();
            m_parameterIndex = AZ::Failure();
        }
        else
        {
            m_parameterIndex = mAnimGraph->FindValueParameterIndexByName(m_parameterName);
        }
    }

    void AnimGraphParameterAction::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context)) {
            serializeContext->Class<AnimGraphParameterAction, AnimGraphTriggerAction>()
                ->Version(2)
                ->Field("m_parameterName", &AnimGraphParameterAction::m_parameterName)
                ->Field("m_parameterValue", &AnimGraphParameterAction::m_parameterValue)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext()) {

                editContext->Class<AnimGraphParameterAction>("Set Parameter Action", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->DataElement(AZ_CRC("AnimGraphParameter", 0x778af55a), &AnimGraphParameterAction::m_parameterName,
                                  "Parameter",
                                  "The parameter to set when this transition is taken.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphParameterAction::Reinit)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->Attribute(AZ_CRC("AnimGraph", 0x0d53d4b3), &AnimGraphParameterAction::GetAnimGraph)

                    ->DataElement(0, &AnimGraphParameterAction::m_parameterValue,
                                  "Value",
                                  "The value to which to set the parameter.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphParameterAction::GetParameterValueVisibility)
                    ->Attribute(AZ::Crc32("ClassSelectionParameters"), &AnimGraphParameterAction::GetParameterValueSelection)
                    ;
            }
        }
    }
} // namespace EMotionFX
