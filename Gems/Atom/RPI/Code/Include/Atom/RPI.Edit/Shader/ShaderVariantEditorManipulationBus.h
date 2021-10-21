#pragma once

#include <AzCore/EBus/EBus.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderVariantEditorManipulationEvents
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            using MutexType = AZStd::mutex;

            using ShaderOptionsAssignment = AZStd::unordered_map<AZStd::string, AZStd::string>;

            // Add the given options assignment as a new variant with a new stable ID to the shadervariantlist of the given shader asset
            virtual void AddOptionsToShaderVariant(AZ::Data::AssetId shaderAssetId, ShaderOptionsAssignment shaderOptionsAssignment) = 0;
        };
        using ShaderVariantEditorManipulationBus = AZ::EBus<ShaderVariantEditorManipulationEvents>;
    }
}
