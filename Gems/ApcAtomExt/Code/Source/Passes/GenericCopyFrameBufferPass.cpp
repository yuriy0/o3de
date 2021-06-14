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

#include "GenericCopyFrameBufferPass.h"
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<GenericCopyFrameBufferPass> GenericCopyFrameBufferPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return RPI::Ptr<GenericCopyFrameBufferPass>(aznew GenericCopyFrameBufferPass(descriptor));
        }

        GenericCopyFrameBufferPass::GenericCopyFrameBufferPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }
    }   // namespace RPI
}   // namespace AZ
