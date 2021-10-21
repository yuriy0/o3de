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

#include "GenericScreenSpaceBlurPass.h"
#include "GenericScreenSpaceBlurChildPass.h"

#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<GenericScreenSpaceBlurPass> GenericScreenSpaceBlurPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<GenericScreenSpaceBlurPass> pass = aznew GenericScreenSpaceBlurPass(descriptor);
            return AZStd::move(pass);
        }

        GenericScreenSpaceBlurPass::GenericScreenSpaceBlurPass(const RPI::PassDescriptor& descriptor)
            : RPI::ParentPass(descriptor)
        {
        }

        void GenericScreenSpaceBlurPass::ResetInternal()
        {
            RemoveChildren();
        }

        void GenericScreenSpaceBlurPass::CreateChildPasses(uint32_t numBlurMips)
        {
            RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();

            m_verticalBlurChildPasses.clear();
            m_horizontalBlurChildPasses.clear();

            // load shaders
            AZStd::string verticalBlurShaderFilePath = "Shaders/GenericScreenSpaceBlurVertical.azshader";
            Data::Instance<AZ::RPI::Shader> verticalBlurShader = RPI::LoadShader(verticalBlurShaderFilePath);
            if (verticalBlurShader == nullptr)
            {
                AZ_Error("PassSystem", false, "[GenericScreenSpaceBlurPass '%s']: Failed to load shader '%s'!", GetPathName().GetCStr(), verticalBlurShaderFilePath.c_str());
                return;
            }

            AZStd::string horizontalBlurShaderFilePath = "Shaders/GenericScreenSpaceBlurHorizontal.azshader";
            Data::Instance<AZ::RPI::Shader> horizontalBlurShader = RPI::LoadShader(horizontalBlurShaderFilePath);
            if (horizontalBlurShader == nullptr)
            {
                AZ_Error("PassSystem", false, "[GenericScreenSpaceBlurPass '%s']: Failed to load shader '%s'!", GetPathName().GetCStr(), horizontalBlurShaderFilePath.c_str());
                return;
            }

            // load pass templates
            const AZStd::shared_ptr<RPI::PassTemplate> blurVerticalPassTemplate = RPI::PassSystemInterface::Get()->GetPassTemplate(Name("GenericScreenSpaceBlurVerticalPassTemplate"));
            const AZStd::shared_ptr<RPI::PassTemplate> blurHorizontalPassTemplate = RPI::PassSystemInterface::Get()->GetPassTemplate(Name("GenericScreenSpaceBlurHorizontalPassTemplate"));

            // create pass descriptors
            RPI::PassDescriptor verticalBlurChildDesc;
            verticalBlurChildDesc.m_passTemplate = blurVerticalPassTemplate;

            RPI::PassDescriptor horizontalBlurChildDesc;
            horizontalBlurChildDesc.m_passTemplate = blurHorizontalPassTemplate;

            // add child passes to perform the vertical and horizontal Gaussian blur for each roughness mip level
            for (uint32_t mip = 0; mip < numBlurMips; ++mip)
            {
                // create Vertical blur child passes
                {
                    AZStd::string verticalBlurChildPassName = AZStd::string::format("GenericScreenSpaceBlur_VerticalMip%d", mip + 1);
                    verticalBlurChildDesc.m_passName = Name(verticalBlurChildPassName);

                    RPI::Ptr<GenericScreenSpaceBlurChildPass> verticalBlurChildPass = passSystem->CreatePass<GenericScreenSpaceBlurChildPass>(verticalBlurChildDesc);
                    verticalBlurChildPass->SetType(Render::GenericScreenSpaceBlurChildPass::PassType::Vertical);
                    verticalBlurChildPass->SetMipLevel(mip + 1);
                    m_verticalBlurChildPasses.push_back(verticalBlurChildPass);

                    AddChild(verticalBlurChildPass);
                }

                // create Horizontal blur child passes
                {
                    AZStd::string horizontalBlurChildPassName = AZStd::string::format("GenericScreenSpaceBlur_HorizonalMip%d", mip + 1);
                    horizontalBlurChildDesc.m_passName = Name(horizontalBlurChildPassName);

                    RPI::Ptr<Render::GenericScreenSpaceBlurChildPass> horizontalBlurChildPass = passSystem->CreatePass<GenericScreenSpaceBlurChildPass>(horizontalBlurChildDesc);
                    horizontalBlurChildPass->SetType(Render::GenericScreenSpaceBlurChildPass::PassType::Horizontal);
                    horizontalBlurChildPass->SetMipLevel(mip + 1);
                    m_horizontalBlurChildPasses.push_back(horizontalBlurChildPass);

                    AddChild(horizontalBlurChildPass);
                }
            }
        }

        void GenericScreenSpaceBlurPass::BuildInternal()
        {
            RemoveChildren();
            m_flags.m_createChildren = true;
            
            // update the image attachment descriptor to sync up size and format
            auto& inputOutputAttachment = GetInputOutputBinding(0).m_attachment;
            inputOutputAttachment->Update();
            const uint32_t mipLevels = inputOutputAttachment->m_descriptor.m_image.m_mipLevels;
            const RHI::Size imageSize = inputOutputAttachment->m_descriptor.m_image.m_size;

            // create transient attachments, one for each blur mip level
            AZStd::vector<RPI::Ptr<RPI::PassAttachment>> transientPassAttachments;
            for (uint32_t mip = 1; mip <= mipLevels - 1; ++mip)
            {
                RHI::Size mipSize = imageSize.GetReducedMip(mip);

                RHI::ImageBindFlags imageBindFlags = RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite;
                auto transientImageDesc = RHI::ImageDescriptor::Create2D(imageBindFlags, mipSize.m_width, mipSize.m_height, RHI::Format::R16G16B16A16_FLOAT);

                RPI::PassAttachment* transientPassAttachment = aznew RPI::PassAttachment();
                AZStd::string transientAttachmentName = AZStd::string::format("GenericScreenSpace_BlurImage%d", mip);
                transientPassAttachment->m_name = transientAttachmentName;
                transientPassAttachment->ComputePathName(GetPathName());
                transientPassAttachment->m_lifetime = RHI::AttachmentLifetimeType::Transient;
                transientPassAttachment->m_descriptor = transientImageDesc;
                transientPassAttachment->m_ownerPass = this;

                transientPassAttachments.push_back(transientPassAttachment);
                m_ownedAttachments.push_back(transientPassAttachment);
            }

            // create child passes, one vertical and one horizontal blur per mip level
            CreateChildPasses(mipLevels - 1);

            // call ParentPass::BuildAttachmentsInternal() first to configure the slots and auto-add the empty bindings,
            // then we will assign attachments to the bindings
            ParentPass::BuildInternal();

            // setup attachment bindings on vertical blur child passes
            uint32_t attachmentIndex = 0;
            for (auto& verticalBlurChildPass : m_verticalBlurChildPasses)
            {
                RPI::PassAttachmentBinding& inputAttachmentBinding = verticalBlurChildPass->GetInputOutputBinding(0);
                inputAttachmentBinding.SetAttachment(inputOutputAttachment);
                inputAttachmentBinding.m_connectedBinding = &GetInputOutputBinding(0);

                RPI::PassAttachmentBinding& outputAttachmentBinding = verticalBlurChildPass->GetInputOutputBinding(1);
                outputAttachmentBinding.SetAttachment(transientPassAttachments[attachmentIndex]);

                attachmentIndex++;
            }
 
            // setup attachment bindings on horizontal blur child passes
            attachmentIndex = 0;
            for (auto& horizontalBlurChildPass : m_horizontalBlurChildPasses)
            {
                RPI::PassAttachmentBinding& inputAttachmentBinding = horizontalBlurChildPass->GetInputOutputBinding(0);
                inputAttachmentBinding.SetAttachment(transientPassAttachments[attachmentIndex]);

                RPI::PassAttachmentBinding& outputAttachmentBinding = horizontalBlurChildPass->GetInputOutputBinding(1);
                uint32_t mipLevel = attachmentIndex + 1;
                RHI::ImageViewDescriptor outputViewDesc;
                outputViewDesc.m_mipSliceMin = static_cast<uint16_t>(mipLevel);
                outputViewDesc.m_mipSliceMax = static_cast<uint16_t>(mipLevel);
                outputAttachmentBinding.m_unifiedScopeDesc.SetAsImage(outputViewDesc);
                outputAttachmentBinding.SetAttachment(inputOutputAttachment);

                attachmentIndex++;
            }
        }
    }   // namespace RPI
}   // namespace AZ
