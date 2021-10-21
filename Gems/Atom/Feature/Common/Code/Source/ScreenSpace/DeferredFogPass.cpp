/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScreenSpace/DeferredFogPass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

namespace AZ
{
    namespace Render
    {
        template<class Derived, class BasePass>
        DeferredFogPass_tpl<Derived, BasePass>::DeferredFogPass_tpl(const RPI::PassDescriptor& descriptor)
            : BasePass(descriptor)
        {
            DeferredFogGlobalNotificationBus::Handler::BusConnect();
        }

        template<class Derived, class BasePass>
        DeferredFogPass_tpl<Derived, BasePass>::~DeferredFogPass_tpl()
        {
            DeferredFogGlobalNotificationBus::Handler::BusDisconnect();
        }

        template<class Derived, class BasePass>
        RPI::Ptr<Derived> DeferredFogPass_tpl<Derived, BasePass>::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<Derived> pass = aznew Derived(descriptor);
            pass->SetSrgBindIndices();

            return AZStd::move(pass);
        }


        template<class Derived, class BasePass>
        void DeferredFogPass_tpl<Derived, BasePass>::InitDeferredFogPass()
        {
        }

        //---------------------------------------------------------------------
        //! Setting and Binding Shader SRG Constants using settings macro reflection

        template<class Derived, class BasePass>
        const DeferredFogSettings* DeferredFogPass_tpl<Derived, BasePass>::GetPassFogSettings() const
        {
            RPI::Scene* scene = BasePass::GetScene();
            if (!scene)
            {
                return nullptr;
            }

            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    return postProcessSettings->GetDeferredFogSettings();
                }
            }
            return nullptr;
        }


        //! Set the binding indices of all members of the SRG
        template<class Derived, class BasePass>
        void DeferredFogPass_tpl<Derived, BasePass>::SetSrgBindIndices()
        {
            Data::Instance<RPI::ShaderResourceGroup> srg = BasePass::m_shaderResourceGroup.get();
            
            // match and set all SRG constants' indices
#define AZ_GFX_COMMON_PARAM(ValueType, FunctionName, MemberName, DefaultValue)                          \
            m_instanceData.MemberName##SrgIndex = srg->FindShaderInputConstantIndex(Name(#MemberName));   \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
            // For texture use a different function call
#undef  AZ_GFX_TEXTURE2D_PARAM
#define AZ_GFX_TEXTURE2D_PARAM(FunctionName, MemberName, DefaultValue)                                  \
            m_instanceData.MemberName##SrgIndex = srg->FindShaderInputImageIndex(Name(#MemberName));      \

#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            m_srgBindIndicesInitialized = true;
        }


        //! Bind SRG constants - done via macro reflection
        template<class Derived, class BasePass>
        void DeferredFogPass_tpl<Derived, BasePass>::SetSrgConstants(const DeferredFogSettings& fogSettings)
        {
            Data::Instance<RPI::ShaderResourceGroup> srg = BasePass::m_shaderResourceGroup.get();

            if (!m_srgBindIndicesInitialized)
            {   // Should be initialize before, but if not - this is a fail safe that will apply it once
                SetSrgBindIndices();
            }

            if (m_settingsDirty)
            {   // SRG constants are up to date and will be bound as they are.
                // First time around they will be dirty to ensure properly set. 

                // Load all texture resources:
                //  first set all macros to be empty, but override the texture for setting images.
#include <Atom/Feature/ParamMacros/MapParamEmpty.inl>

#undef  AZ_GFX_TEXTURE2D_PARAM
#define AZ_GFX_TEXTURE2D_PARAM(Name, MemberName, DefaultValue)                  \
                {\
                    m_instanceData.MemberName##Image =                                \
                        fogSettings.LoadStreamingImage( fogSettings.MemberName.c_str(), "DeferredFogSettings" );  \
                }\

#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

                m_settingsDirty = false;
            }

            // The Srg constants value settings
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                          \
            if (m_instanceData.MemberName##SrgIndex.IsValid()) { \
                srg->SetConstant( m_instanceData.MemberName##SrgIndex, fogSettings.MemberName );     \
            } \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>

            // The following macro overrides the regular macro defined above, loads an image and bind it
#undef AZ_GFX_TEXTURE2D_PARAM
#define AZ_GFX_TEXTURE2D_PARAM(Name, MemberName, DefaultValue)                      \
            if (m_instanceData.MemberName##SrgIndex.IsValid() && !srg->SetImage(m_instanceData.MemberName##SrgIndex, m_instanceData.MemberName##Image ))           \
            {                                                                       \
                AZ_Error( "DeferredFogPass_tpl<Derived, BasePass>::SetSrgConstants", false, "Failed to bind SRG image for %s = %s",  \
                    #MemberName, fogSettings.MemberName.c_str() );                                      \
            }                                                                       \

#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        }
        //---------------------------------------------------------------------

        template<class Derived, class BasePass>
        bool DeferredFogPass_tpl<Derived, BasePass>::IsEnabled() const 
        {
            const DeferredFogSettings* fogSettings = GetPassFogSettings();
            return BasePass::IsEnabled() && (fogSettings ? fogSettings->GetEnabled() : false);
        }

        template<class Derived, class BasePass>
        void DeferredFogPass_tpl<Derived, BasePass>::UpdateShaderOptions(const DeferredFogSettings& fogSettings)
        {
            static const auto BoolToShaderOption = [](bool x)
            {
                return x ? AZ::Name("true") : AZ::Name("false");
            };

            RPI::ShaderOptionGroup shaderOption = BasePass::m_shader->CreateShaderOptionGroup();

            static const AZ::Name o_enableFogLayer("o_enableFogLayer");
            static const AZ::Name o_useNoiseTexture("o_useNoiseTexture");

            // [TODO][ATOM-13659] - AZ::Name all over our code base should use init with string and
            // hash key for the iterations themselves.
            if (shaderOption.FindShaderOptionIndex(o_enableFogLayer).IsValid())
            {
                shaderOption.SetValue(o_enableFogLayer, BoolToShaderOption(fogSettings.GetEnableFogLayerShaderOption()));
            }
            if (shaderOption.FindShaderOptionIndex(o_useNoiseTexture).IsValid())
            {
                shaderOption.SetValue(o_useNoiseTexture, BoolToShaderOption(fogSettings.GetUseNoiseTextureShaderOption()));
            }

            // The following method returns the specified options, as well as fall back values for all 
            // non-specified options.  If all were set you can use the method GetShaderVariantKey that is 
            // cheaper but will not make sure the populated values has the default fall back for any unset bit.
            m_ShaderOptions = shaderOption.GetShaderVariantKeyFallbackValue();
        }

        template<class Derived, class BasePass>
        void DeferredFogPass_tpl<Derived, BasePass>::OnDeferredFogGlobalSettingsChanged()
        {
            m_settingsDirty = true;
        }

        template<class Derived, class BasePass>
        void DeferredFogPass_tpl<Derived, BasePass>::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            BasePass::SetupFrameGraphDependencies(frameGraph);
            UpdateDeferredFogPassSrg();
        }

        template<class Derived, class BasePass>
        void DeferredFogPass_tpl<Derived, BasePass>::UpdateDeferredFogPassSrg()
        {
            if (!BasePass::m_pipeline || !BasePass::m_pipeline->GetScene()) return;

            // If any change was made, make sure to bind it.
            if (const DeferredFogSettings* fogSettings = GetPassFogSettings())
            {
                const bool fogIsEnabled = fogSettings->GetEnabled();
                BasePass::SetEnabled(fogIsEnabled);

                if (fogIsEnabled)
                {
                    // Update and set the per pass shader options - this will update the current required
                    // shader variant and if doesn't exist, it will be created via the compile stage
                    if (BasePass::m_shaderResourceGroup->HasShaderVariantKeyFallbackEntry())
                    {
                        UpdateShaderOptions(*fogSettings);
                    }

                    SetSrgConstants(*fogSettings);
                }
            }
            else
            {
                BasePass::SetEnabled(false);
            }
        }

        template<class Derived, class BasePass>
        void DeferredFogPass_tpl<Derived, BasePass>::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            if (BasePass::m_shaderResourceGroup->HasShaderVariantKeyFallbackEntry())
            {
                BasePass::m_shaderResourceGroup->SetShaderVariantKeyFallbackValue(m_ShaderOptions);
            }

            BasePass::CompileResources(context);
        }


        // Concrete pass types
        class DeferredFogFullScreenTrianglePass
            : public DeferredFogPass_tpl<DeferredFogFullScreenTrianglePass, AZ::RPI::FullscreenTrianglePass>
        {
            AZ_RPI_PASS(DeferredFogFullScreenTrianglePass);

        public:
            AZ_RTTI(DeferredFogFullScreenTrianglePass, "{0406C8AB-E95D-43A7-AF53-BDEE22D36746}", RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(DeferredFogFullScreenTrianglePass, SystemAllocator, 0);

            using DeferredFogPass_tpl::DeferredFogPass_tpl;

            void InitializeInternal() override
            {
                FullscreenTrianglePass::InitializeInternal();
                InitDeferredFogPass();
            }
        };


        class DeferredFogComputePass
            : public DeferredFogPass_tpl<DeferredFogComputePass, AZ::RPI::ComputePass>
        {
            AZ_RPI_PASS(DeferredFogComputePass);

        public:
            AZ_RTTI(DeferredFogComputePass, "{2125C260-D2CE-4EB7-AFE4-5164933B1E7C}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(DeferredFogComputePass, SystemAllocator, 0);

            using DeferredFogPass_tpl::DeferredFogPass_tpl;

            void InitializeInternal() override
            {
                ComputePass::InitializeInternal();
                InitDeferredFogPass();
            }

            void CompileResources(const RHI::FrameGraphCompileContext& context) override
            {
                UpdateDeferredFogPassSrg();
                ComputePass::CompileResources(context);
            }
        };

        const AZStd::array<AZStd::pair<AZ::Name, AZ::RPI::PassCreator>, 2>& GetDeferredFogPasses()
        {
            static const AZStd::array<AZStd::pair<AZ::Name, AZ::RPI::PassCreator>, 2> result =
            {{
                { Name("DeferredFogFullScreenTrianglePass"), &DeferredFogFullScreenTrianglePass::Create },
                { Name("DeferredFogComputePass"), &DeferredFogComputePass::Create }
            }};
            return result;
        }

    }   // namespace Render
}   // namespace AZ

