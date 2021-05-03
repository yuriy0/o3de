/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace AZ
{
    namespace Render
    {
        //! This pass takes performs a separable Gaussian blur of the input Generic image
        //! to the lower mip levels of that image.  The blurred mips are used as roughness levels
        //! when applying Generic data to a surface material.
        class GenericScreenSpaceBlurChildPass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(GenericScreenSpaceBlurChildPass);

        public:
            AZ_RTTI(Render::GenericScreenSpaceBlurChildPass, "{2952CAF6-D8AD-499C-8131-FED4B87F1709}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::GenericScreenSpaceBlurChildPass, SystemAllocator, 0);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<GenericScreenSpaceBlurChildPass> Create(const RPI::PassDescriptor& descriptor);

            enum PassType
            {
                Vertical,
                Horizontal
            };

            void SetType(PassType passType) { m_passType = passType; }
            void SetMipLevel(uint32_t mipLevel) { m_mipLevel = mipLevel; }

        private:
            explicit GenericScreenSpaceBlurChildPass(const RPI::PassDescriptor& descriptor);

            // ComputePass Overrides...
            void FrameBeginInternal(FramePrepareParams params) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            bool m_updateSrg = false;
            PassType m_passType;
            uint32_t m_mipLevel = 0;
            RHI::Size m_imageSize;
            float m_outputScale = 0.0f;
        };
    }   // namespace RPI
}   // namespace AZ
