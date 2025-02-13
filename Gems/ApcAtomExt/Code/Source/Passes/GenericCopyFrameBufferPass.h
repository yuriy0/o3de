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

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>

namespace AZ
{
    namespace Render
    {
        //! This pass copies the frame buffer prior to the post-processing pass.
        class GenericCopyFrameBufferPass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(GenericCopyFrameBufferPass);

        public:
            AZ_RTTI(Render::GenericCopyFrameBufferPass, "{25962B14-B159-4FD1-83A2-AEBBEB3CAC2E}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::GenericCopyFrameBufferPass, SystemAllocator, 0);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<GenericCopyFrameBufferPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit GenericCopyFrameBufferPass(const RPI::PassDescriptor& descriptor);
        };
    }   // namespace RPI
}   // namespace AZ
