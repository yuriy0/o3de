#pragma once

#include <AzCore/Component/Component.h>

#include <Atom/RPI.Edit/Shader/ShaderVariantEditorManipulationBus.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderVariantEditorManipulationSystemComponent final
            : public AZ::Component
            , private ShaderVariantEditorManipulationBus::Handler
        {
        public:
            AZ_COMPONENT(ShaderVariantEditorManipulationSystemComponent, "{5274C48A-C548-479E-8C94-837902178597}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

            ShaderVariantEditorManipulationSystemComponent() = default;

            void Activate() override;
            void Deactivate() override;

        private:
            // ShaderVariantEditorManipulationBus
            void AddOptionsToShaderVariant(AZ::Data::AssetId shaderAssetId, ShaderOptionsAssignment shaderOptionsAssignment) override;

            void AddOptionsToShaderVariant_Impl(AZ::Data::AssetId shaderAssetId, ShaderOptionsAssignment shaderOptionsAssignment);

            AZ_DISABLE_COPY_MOVE(ShaderVariantEditorManipulationSystemComponent);
        };
    } // namespace RPI
} // namespace AZ

