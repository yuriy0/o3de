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
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace Render
    {
        //! This pass performs a separable Gaussian blur of the input image to the lower mip levels of that image.
        class GenericScreenSpaceBlurPass
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(GenericScreenSpaceBlurPass);

        public:
            AZ_RTTI(Render::GenericScreenSpaceBlurPass, "{2B239FE5-F496-4852-B937-CD78342BF4C3}", ParentPass);
            AZ_CLASS_ALLOCATOR(Render::GenericScreenSpaceBlurPass, SystemAllocator, 0);
          
            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<GenericScreenSpaceBlurPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit GenericScreenSpaceBlurPass(const RPI::PassDescriptor& descriptor);

            void CreateChildPasses(uint32_t numBlurMips);

            // Pass Overrides...
            void ResetInternal() override;
            void BuildAttachmentsInternal() override;

            AZStd::vector<RPI::Ptr<RPI::FullscreenTrianglePass>> m_verticalBlurChildPasses;
            AZStd::vector<RPI::Ptr<RPI::FullscreenTrianglePass>> m_horizontalBlurChildPasses;
        };
    }   // namespace RPI
}   // namespace AZ
