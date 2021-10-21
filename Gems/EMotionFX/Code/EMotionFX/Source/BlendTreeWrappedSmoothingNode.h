/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"
#include "BlendTreeSmoothingNode.h"

namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeWrappedSmoothingNode
        : public BlendTreeSmoothingNode
    {
    public:
        AZ_RTTI(BlendTreeWrappedSmoothingNode, "{44ECD787-7B16-4D18-BB50-4D452B5DE2F4}", BlendTreeSmoothingNode);
        AZ_CLASS_ALLOCATOR_DECL;

        using UniqueData = BlendTreeSmoothingNode::UniqueData;

        BlendTreeWrappedSmoothingNode() = default;
        ~BlendTreeWrappedSmoothingNode() = default;

        const char* GetPaletteName() const override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        float DoSmoothing(float sourceValue, float destValue, float lerpStrength, UniqueData* uniqueData) override;

    private:
        float               m_minValue = 0.f;
        float               m_maxValue = 1.f;
    };
} // namespace EMotionFX
