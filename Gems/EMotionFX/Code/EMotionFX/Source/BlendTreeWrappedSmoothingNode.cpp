/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeWrappedSmoothingNode.h"

#include <AzCore/Math/Vector2.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeWrappedSmoothingNode, AnimGraphAllocator, 0);

    // get the palette name
    const char* BlendTreeWrappedSmoothingNode::GetPaletteName() const
    {
        return "Wrapped Smoothing";
    }

    float BlendTreeWrappedSmoothingNode::DoSmoothing(float sourceValue, float destValue, float t, UniqueData*)
    {
        if (m_minValue >= m_maxValue || t >= 1.f)
        {
            return destValue;
        }
        else if (t <= 0.f)
        {
            return sourceValue;
        }

        const float wrappedSource = AZ::Wrap(sourceValue, m_minValue, m_maxValue);
        const float wrappedDest = AZ::Wrap(destValue, m_minValue, m_maxValue);
        const float width = m_maxValue - m_minValue;

        float delta = (wrappedDest - wrappedSource) / width;
        if (AZ::GetAbs(delta) > 0.5f)
        {
            //delta = delta > 0.f ? -1.f + delta : 1.f + delta;
            delta += delta > 0.f ? -1.f : 1.f;
        }
        delta *= width;

        return AZ::Wrap(wrappedSource + delta * t, m_minValue, m_maxValue);
    }

    void BlendTreeWrappedSmoothingNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeWrappedSmoothingNode, BlendTreeSmoothingNode>()
            ->Version(1)
            ->Field("minValue", &BlendTreeWrappedSmoothingNode::m_minValue)
            ->Field("maxValue", &BlendTreeWrappedSmoothingNode::m_maxValue)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeWrappedSmoothingNode>("Wrapped Smoothing", "Smoothing attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeWrappedSmoothingNode::m_minValue,
                "Min Value", "Below this value, wraps around to the max")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeWrappedSmoothingNode::m_maxValue,
                "Max Value", "Above this value, wraps around to the min")
        ;
    }
} // namespace EMotionFX
