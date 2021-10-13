/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <ScreenSpace/DeferredFogSettings.h>

namespace AZ
{
    namespace Render
    {
        static const char* const DeferredFogPassTemplateName = "DeferredFogPassTemplate";

        // Deferred screen space fog pass class.  The fog is calculated post
        // the main render using the linear depth and turbulence texture with two blended
        // octaves that emulate the fog thickness and fog animation along the view ray direction.
        // The fog can be a full screen fog or a thin 3D layer fog representing mountains morning mist.
        // The pass also exposes the fog settings to be used by an editor component node that will
        // control the visual properties of the fog.
        // Enhancements of this fog can contain more advanced noise handling (real volumetric), areal
        // mask, blending between areal fog nodes and other enhancements required for production.

        template<class Derived, class BasePass>
        class DeferredFogPass_tpl
            : public BasePass
            , private DeferredFogGlobalNotificationBus::Handler
        {
        public:
            ~DeferredFogPass_tpl();

            static RPI::Ptr<Derived> Create(const RPI::PassDescriptor& descriptor);

            const DeferredFogSettings* GetPassFogSettings() const;

            virtual bool IsEnabled() const override;

            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

        protected:
            DeferredFogPass_tpl(const RPI::PassDescriptor& descriptor);

            void InitDeferredFogPass();

            void UpdateDeferredFogPassSrg();

            //! Set the binding indices of all members of the SRG
            void SetSrgBindIndices();

            //! Bind SRG constants - done via macro reflection
            void SetSrgConstants(const DeferredFogSettings& fogSettings);

            void UpdateShaderOptions(const DeferredFogSettings& fogSettings);

        private:
            // DeferredFogGlobalNotificationBus
            void OnDeferredFogGlobalSettingsChanged() override;

            // Per-Instance data
            DeferredFogSettings::SrgInstanceData m_instanceData;

            // Shader options for variant generation (texture and layer activation in this case)
            AZ::RPI::ShaderVariantKey m_ShaderOptions;

            bool m_srgBindIndicesInitialized = false;
            AZStd::atomic_bool m_settingsDirty = false;
        };

        const AZStd::array<AZStd::pair<AZ::Name, AZ::RPI::PassCreator>, 2>& GetDeferredFogPasses();

    }   // namespace Render
}   // namespace AZ
