/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZ_MONOLITHIC_BUILD)

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Module/Environment.h>
#include <SceneAPI/SceneBuilder/SceneImportRequestHandler.h>

#include <SceneAPI/SceneBuilder/SceneImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpBitangentStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpColorStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpMaterialImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpMeshImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpTangentStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpTransformImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpUvMapImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpSkinImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpSkinWeightsImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpBoneImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpAnimationImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpBlendShapeImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxAnimationImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxBlendShapeImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxBoneImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxColorStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxTangentStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxBitangentStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxMaterialImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxMeshImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxSkinImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxSkinWeightsImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxTransformImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxUvMapImporter.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            static AZStd::vector<AZ::ComponentDescriptor*> g_componentDescriptors;

            void Reflect(AZ::SerializeContext* /*context*/)
            {
                // Descriptor registration is done in Reflect instead of Initialize because the ResourceCompilerScene initializes the libraries before
                // there's an application.
                using namespace AZ::SceneAPI;
                using namespace AZ::SceneAPI::SceneBuilder;

                if (g_componentDescriptors.empty())
                {
                    // Global importer and behavior
                    g_componentDescriptors.push_back(SceneBuilder::SceneImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(SceneImportRequestHandler::CreateDescriptor());

                    // Node and attribute importers
                    g_componentDescriptors.push_back(FbxAnimationImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxBlendShapeImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxBoneImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxColorStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxMaterialImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxMeshImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxSkinImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxSkinWeightsImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxTransformImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxUvMapImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxTangentStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxBitangentStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpBitangentStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpColorStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpMaterialImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpMeshImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpTangentStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpTransformImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpUvMapImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpSkinImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpSkinWeightsImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpBoneImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpAnimationImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpBlendShapeImporter::CreateDescriptor());

                    for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
                    {
                        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Handler::RegisterComponentDescriptor, descriptor);
                    }
                }
            }

            void ReflectBehavior([[maybe_unused]] AZ::BehaviorContext* context)
            {
                // stub in until LYN-1284 is done
            }

            void Activate()
            {
            }
            
            void Deactivate()
            {
            }

            void Uninitialize()
            {
                if (!g_componentDescriptors.empty())
                {
                    for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
                    {
                        descriptor->ReleaseDescriptor();
                    }
                    g_componentDescriptors.clear();
                    g_componentDescriptors.shrink_to_fit();
                }
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ

extern "C" AZ_DLL_EXPORT void InitializeDynamicModule(void* env)
{
    AZ::Environment::Attach(static_cast<AZ::EnvironmentInstance>(env));
}
extern "C" AZ_DLL_EXPORT void Reflect(AZ::SerializeContext* context)
{
    AZ::SceneAPI::SceneBuilder::Reflect(context);
}
extern "C" AZ_DLL_EXPORT void ReflectBehavior(AZ::BehaviorContext* context)
{
    AZ::SceneAPI::SceneBuilder::ReflectBehavior(context);
}
extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule()
{
    AZ::SceneAPI::SceneBuilder::Uninitialize();
    AZ::Environment::Detach();
}

#endif // !defined(AZ_MONOLITHIC_BUILD)
