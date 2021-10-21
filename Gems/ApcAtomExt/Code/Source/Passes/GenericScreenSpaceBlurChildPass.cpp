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

#include "GenericScreenSpaceBlurChildPass.h"
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<GenericScreenSpaceBlurChildPass> GenericScreenSpaceBlurChildPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<GenericScreenSpaceBlurChildPass> pass = aznew GenericScreenSpaceBlurChildPass(descriptor);
            return AZStd::move(pass);
        }

        GenericScreenSpaceBlurChildPass::GenericScreenSpaceBlurChildPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void GenericScreenSpaceBlurChildPass::FrameBeginInternal(FramePrepareParams params)
        {
            // get attachment size
            RPI::PassAttachment* inputAttachment = GetInputOutputBinding(0).m_attachment.get();
            AZ_Assert(inputAttachment, "GenericScreenSpaceBlurChildPass: Input binding has no attachment!");

            RHI::Size size = inputAttachment->m_descriptor.m_image.m_size;

            if (m_imageSize != size)
            {
                m_imageSize = size;
                m_outputScale = (m_passType == PassType::Vertical) ? powf(2.0f, static_cast<float>(m_mipLevel)) : 1.0f;

                m_updateSrg = true;
            }

            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        void GenericScreenSpaceBlurChildPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            if (m_updateSrg)
            {
                RHI::ShaderInputConstantIndex constantIndex;

                constantIndex = m_shaderResourceGroup->GetLayout()->FindShaderInputConstantIndex(AZ::Name("m_imageWidth"));
                m_shaderResourceGroup->SetConstant(constantIndex, m_imageSize.m_width);

                constantIndex = m_shaderResourceGroup->GetLayout()->FindShaderInputConstantIndex(AZ::Name("m_imageHeight"));
                m_shaderResourceGroup->SetConstant(constantIndex, m_imageSize.m_height);

                constantIndex = m_shaderResourceGroup->GetLayout()->FindShaderInputConstantIndex(AZ::Name("m_outputScale"));
                m_shaderResourceGroup->SetConstant(constantIndex, m_outputScale);

                m_updateSrg = false;
            }

            FullscreenTrianglePass::CompileResources(context);
        }

    }   // namespace RPI
}   // namespace AZ
