#include "ShaderVariantEditorManipulationSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderVariantEditorManipulationSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<ShaderVariantEditorManipulationSystemComponent, Component>()
                    ->Version(1)
                    ;

                if (AZ::EditContext* ec = serializeContext->GetEditContext())
                {
                    ec->Class<ShaderVariantEditorManipulationSystemComponent>("ShaderVariantEditorManipulationSystemComponent", "Atom Renderer")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void ShaderVariantEditorManipulationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ShaderVariantEditorManipulationSystemService"));
        }

        void ShaderVariantEditorManipulationSystemComponent::Activate()
        {
            ShaderVariantEditorManipulationBus::Handler::BusConnect();
        }

        void ShaderVariantEditorManipulationSystemComponent::Deactivate()
        {
            ShaderVariantEditorManipulationBus::Handler::BusDisconnect();
        }


        template<class K, class V>
        bool unordered_map_equals(const AZStd::unordered_map<K, V>& m0, const AZStd::unordered_map<K, V>& m1)
        {
            if (m0.size() != m1.size()) return false;

            for (const auto& kv : m0)
            {
                auto it = m1.find(kv.first);
                if (it == m1.end()) return false;
                if (kv.second != it->second) return false;
            }

            return true;
        }

        void ShaderVariantEditorManipulationSystemComponent::AddOptionsToShaderVariant(AZ::Data::AssetId shaderAssetId, ShaderOptionsAssignment shaderOptionsAssignment)
        {
            if (shaderOptionsAssignment.empty())
            {
                AZ_Warning("ShaderVariantEditorManipulationBus", false, "Cannot add variant with no options assignments");
                return;
            }

            AZ::CreateJobFunction(
                [=]() {
                    AddOptionsToShaderVariant_Impl(AZStd::move(shaderAssetId), AZStd::move(shaderOptionsAssignment));
                },
                true
            )->Start();
        }

        void ShaderVariantEditorManipulationSystemComponent::AddOptionsToShaderVariant_Impl(AZ::Data::AssetId shaderAssetId, ShaderOptionsAssignment shaderOptionsAssignment)
        {
            using AZ::Data::AssetCatalogRequestBus;
            using AzToolsFramework::AssetSystemRequestBus;

            // Convert the shader asset ID to a shader list variant asset ID
            AZStd::string shaderAssetPath;
            AssetCatalogRequestBus::BroadcastResult(shaderAssetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, shaderAssetId);
            if (shaderAssetPath.empty())
            {
                AZ_Warning("ShaderVariantEditorManipulationBus", false, "Could not find path for shader asset %s", shaderAssetId.ToString<AZStd::string>().c_str());
                return;
            }

            const AZStd::string shaderAssetPath_copy = shaderAssetPath;
            bool success = false;
            AssetSystemRequestBus::BroadcastResult(success,
                &AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath,
                shaderAssetPath_copy, shaderAssetPath
            );
            if (shaderAssetPath.empty())
            {
                AZ_Warning("ShaderVariantEditorManipulationBus", false, "Could not find path for shader asset %s", shaderAssetId.ToString<AZStd::string>().c_str());
                return;
            }

            AZ::StringFunc::Path::StripExtension(shaderAssetPath);
            shaderAssetPath += ".shadervariantlist";

            // Load the shader variant list source data
            ShaderVariantListSourceData shaderVariantList;
            if (!RPI::JsonUtils::LoadObjectFromFile(shaderAssetPath, shaderVariantList))
            {
                AZ_Warning("ShaderVariantEditorManipulationBus", false, "Could not find .shadervariantlist asset at path %s", shaderAssetPath.c_str());
                return;
            }

            // Check if this variant already exists in the map, and also find a new stable ID to use if it doesn't
            AZStd::bitset<ShaderVariantKeyBitCount> usedStableVariant;
            bool variantExists = false;
            for (const auto& variant : shaderVariantList.m_shaderVariants)
            {
                if (!variantExists && unordered_map_equals(variant.m_options, shaderOptionsAssignment))
                {
                    variantExists = true;
                }

                usedStableVariant.set(variant.m_stableId);
            }

            if (variantExists)
            {
                //AZ_Printf("ShaderVariantEditorManipulationBus", "Skipped adding variant, already exists");
                return;
            }

            AZStd::optional<size_t> nextStableVariantKey;

            // 0 has a special meaning (root variant) and may not be used, so start searching at 1
            for (size_t i = 1; i < ShaderVariantKeyBitCount; ++i)
            {
                if (!usedStableVariant.test(i))
                {
                    nextStableVariantKey = i;
                    break;
                }
            }

            // 0 has a special meaning (root variant) and may not be used
            if (!nextStableVariantKey)
            {
                AZ_Warning("ShaderVariantEditorManipulationBus", false, "No more stable variant keys available for %s", shaderAssetPath.c_str());
                return;
            }

            // Add the variant
            shaderVariantList.m_shaderVariants.push_back();
            auto& variant = shaderVariantList.m_shaderVariants.back();
            variant.m_options = AZStd::move(shaderOptionsAssignment);
            variant.m_stableId = static_cast<AZ::u32>(*nextStableVariantKey);

            // Request edit file from source control
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
                shaderAssetPath.c_str(), true, [](bool, const AzToolsFramework::SourceControlFileInfo&) {});

            // Save out data
            if (!RPI::JsonUtils::SaveObjectToFile(shaderAssetPath, shaderVariantList))
            {
                AZ_Warning("ShaderVariantEditorManipulationBus", false, "Could not save .shadervariantlist asset at path %s", shaderAssetPath.c_str());
                return;
            }
        };
    }
}
