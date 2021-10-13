/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/SceneImporter.h>
#include <SceneAPI/SceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpMaterialImporter.h>
#include <SceneAPI/SceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/FbxSDKWrapper/FbxSceneWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SceneBuilder/ImportContexts/FbxImportContexts.h>

// APC BEGIN
#include <AzCore/Settings/SettingsRegistry.h>
// APC END

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            struct QueueNode
            {
                std::shared_ptr<SDKNode::NodeWrapper> m_node;
                Containers::SceneGraph::NodeIndex m_parent;

                QueueNode() = delete;
                QueueNode(std::shared_ptr<SDKNode::NodeWrapper>&& node, Containers::SceneGraph::NodeIndex parent)
                    : m_node(std::move(node))
                    , m_parent(parent)
                {
                }
            };

            SceneImporter::SceneImporter()
                : m_sceneSystem(new SceneSystem())
                // APC BEGIN: Use FBX SDK, not AssImp
                , m_useAssetImporterSDK(true)
                // APC END
            {
                // APC BEGIN: allow switching between Assimp/FbxSDK by registry key 
                if (auto registry = AZ::SettingsRegistry::Get())
                {
                    constexpr AZStd::string_view UseAssimpRegistryKey = "/Amazon/AzCore/Bootstrap/UseAssimp";
                    registry->Get(m_useAssetImporterSDK, UseAssimpRegistryKey);
                }
                // APC END

                AZStd::string sceneWrapperName;
                if (m_useAssetImporterSDK)
                {
                    m_sceneWrapper = AZStd::make_unique<AssImpSDKWrapper::AssImpSceneWrapper>();
                    sceneWrapperName = "AssImpSceneWrapper";
                }
                else
                {
                    m_sceneWrapper = AZStd::make_unique<FbxSDKWrapper::FbxSceneWrapper>();
                    sceneWrapperName = "FbxSceneWrapper";
                }
                AZ_Printf("FbxImporter", "Use %s to import FBX file", sceneWrapperName.c_str());

                BindToCall(&SceneImporter::ImportProcessing);
            }

            void SceneImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<SceneImporter, SceneCore::LoadingComponent>()->Version(2); // SPEC-5776
                }
            }

            Events::ProcessingResult SceneImporter::ImportProcessing(Events::ImportEventContext& context)
            {
                m_sceneWrapper->Clear();

                if (!m_sceneWrapper->LoadSceneFromFile(context.GetInputDirectory().c_str()))
                {
                    return Events::ProcessingResult::Failure;
                }

                typedef AZStd::function<bool(Containers::Scene & scene)> ConvertFunc;
                ConvertFunc convertFunc;
                m_sceneSystem->Set(m_sceneWrapper.get());
                if (azrtti_istypeof<FbxSDKWrapper::FbxSceneWrapper>(m_sceneWrapper.get()))
                {
                    convertFunc = AZStd::bind(&SceneImporter::ConvertFbxSceneContext, this, AZStd::placeholders::_1);
                }
                else
                {
                    convertFunc = AZStd::bind(&SceneImporter::ConvertScene, this, AZStd::placeholders::_1);
                }

                if (convertFunc(context.GetScene()))
                {
                    return Events::ProcessingResult::Success;
                }
                else
                {
                    return Events::ProcessingResult::Failure;
                }
            }

            bool SceneImporter::ConvertFbxSceneContext(Containers::Scene& scene) const
            {
                using namespace SceneBuilder;

                std::shared_ptr<SDKNode::NodeWrapper> fbxRoot = m_sceneWrapper->GetRootNode();
                if (!fbxRoot)
                {
                    return false;
                }
                FbxSDKWrapper::FbxSceneWrapper* fbxSceneWrapper = azrtti_cast <FbxSDKWrapper::FbxSceneWrapper*>(m_sceneWrapper.get());
                int sign = 0;
                FbxSDKWrapper::FbxAxisSystemWrapper::UpVector upVector = fbxSceneWrapper->GetAxisSystem()->GetUpVector(sign);
                AZ_Assert(sign != 0, "sign failed to populate which is a failure in GetUpVector");

                if (upVector == FbxSDKWrapper::FbxAxisSystemWrapper::UpVector::Z)
                {
                    if (sign > 0)
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::ZUp);
                    }
                    else
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::NegZUp);
                        AZ_Assert(false, "Negative Z Up scene orientation is not a currently supported orientation.");
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Negative Z Up scene orientation is not a currently supported orientation.");
                    }
                }
                else if (upVector == FbxSDKWrapper::FbxAxisSystemWrapper::UpVector::Y)
                {
                    if (sign > 0)
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::YUp);
                    }
                    else
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::NegYUp);
                        AZ_Assert(false, "Negative Y Up scene orientation is not a currently supported orientation.");
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Negative Y Up scene orientation is not a currently supported orientation.");
                    }
                }
                else if (upVector == FbxSDKWrapper::FbxAxisSystemWrapper::UpVector::X)
                {
                    if (sign > 0)
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::XUp);
                        AZ_Assert(false, "Positive X Up scene orientation is not a currently supported orientation.");
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Positive X Up scene orientation is not a currently supported orientation.");
                    }
                    else
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::NegXUp);
                        AZ_Assert(false, "Negative X Up scene orientation is not a currently supported orientation.");
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Negative X Up scene orientation is not a currently supported orientation.");
                    }
                }

                AZStd::queue<SceneBuilder::QueueNode> nodes;
                nodes.emplace(AZStd::move(fbxRoot), scene.GetGraph().GetRoot());

                RenamedNodesMap nodeNameMap;
                
                while (!nodes.empty())
                {
                    SceneBuilder::QueueNode& node = nodes.front();

                    AZ_Assert(node.m_node, "Empty fbx node queued");
                    
                    if (!nodeNameMap.RegisterNode(node.m_node, scene.GetGraph(), node.m_parent))
                    {
                        AZ_TracePrintf(Utilities::ErrorWindow, "Failed to register fbx node in name table.");
                        continue;
                    }
                    AZStd::string nodeName = nodeNameMap.GetNodeName(node.m_node);
                    AZ_TraceContext("SceneAPI Node Name", nodeName);

                    Containers::SceneGraph::NodeIndex newNode = scene.GetGraph().AddChild(node.m_parent, nodeName.c_str());
                    AZ_Assert(newNode.IsValid(), "Failed to add node to scene graph");
                    if (!newNode.IsValid())
                    {
                        continue;
                    }
                    FbxNodeEncounteredContext sourceNodeEncountered(scene, newNode, *fbxSceneWrapper, *m_sceneSystem, nodeNameMap, *azrtti_cast<AZ::FbxSDKWrapper::FbxNodeWrapper*>(node.m_node.get()));
                    Events::ProcessingResultCombiner nodeResult;
                    nodeResult += Events::Process(sourceNodeEncountered);

                    // If no importer created data, we still create an empty node that may eventually contain a transform
                    if (sourceNodeEncountered.m_createdData.empty())
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Success,
                            "Importers returned success but no data was created");
                        AZStd::shared_ptr<DataTypes::IGraphObject> nullData(nullptr);
                        sourceNodeEncountered.m_createdData.emplace_back(nullData);
                        nodeResult += Events::ProcessingResult::Success;
                    }

                    // Create single node since only one piece of graph data was created
                    if (sourceNodeEncountered.m_createdData.size() == 1)
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Ignored, 
                            "An importer created data, but did not return success");
                        if (nodeResult.GetResult() == Events::ProcessingResult::Failure)
                        {
                            AZ_TracePrintf(Utilities::ErrorWindow, "One or more importers failed to create data.");
                        }

                        SceneDataPopulatedContext dataProcessed(sourceNodeEncountered, 
                            sourceNodeEncountered.m_createdData[0], nodeName.c_str());
                        Events::ProcessingResult result = AddDataNodeWithContexts(dataProcessed);
                        if (result != Events::ProcessingResult::Failure)
                        {
                            newNode = dataProcessed.m_currentGraphPosition;
                        }
                    }
                    // Create an empty parent node and place all data under it. The remaining
                    // tree will be built off of this as the logical parent
                    else
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Ignored,
                            "%i importers created data, but did not return success", 
                            sourceNodeEncountered.m_createdData.size());
                        if (nodeResult.GetResult() == Events::ProcessingResult::Failure)
                        {
                            AZ_TracePrintf(Utilities::ErrorWindow, "One or more importers failed to create data.");
                        }

                        size_t offset = nodeName.length();
                        for (size_t i = 0; i < sourceNodeEncountered.m_createdData.size(); ++i)
                        {
                            nodeName += '_';
                            nodeName += AZStd::to_string(aznumeric_cast<AZ::u64>(i + 1));

                            Containers::SceneGraph::NodeIndex subNode =
                                scene.GetGraph().AddChild(newNode, nodeName.c_str());
                            AZ_Assert(subNode.IsValid(), "Failed to create new scene sub node");
                            SceneDataPopulatedContext dataProcessed(sourceNodeEncountered, 
                                sourceNodeEncountered.m_createdData[i], nodeName);
                            dataProcessed.m_currentGraphPosition = subNode;
                            AddDataNodeWithContexts(dataProcessed);

                            // Remove the temporary extension again.
                            nodeName.erase(offset, nodeName.length() - offset);
                        }
                    }

                    AZ_Assert(nodeResult.GetResult() == Events::ProcessingResult::Success, 
                        "No importers successfully added processed scene data.");
                    AZ_Assert(newNode != node.m_parent, 
                        "Failed to update current graph position during data processing.");

                    int childCount = node.m_node->GetChildCount();
                    for (int i = 0; i < childCount; ++i)
                    {
                        std::shared_ptr<FbxSDKWrapper::FbxNodeWrapper> child = std::make_shared<FbxSDKWrapper::FbxNodeWrapper>(node.m_node->GetChild(i)->GetFbxNode());
                        if (child)
                        {
                            nodes.emplace(AZStd::move(child), newNode);
                        }
                    }

                    nodes.pop();
                }

                Events::ProcessingResult result = Events::Process<FinalizeSceneContext>(scene, *fbxSceneWrapper, *m_sceneSystem, nodeNameMap);
                if (result == Events::ProcessingResult::Failure)
                {
                    return false;
                }

                return true;
            }

            bool SceneImporter::ConvertScene(Containers::Scene& scene) const
            {
                std::shared_ptr<SDKNode::NodeWrapper> sceneRoot = m_sceneWrapper->GetRootNode();
                if (!sceneRoot)
                {
                    return false;
                }

                const AssImpSDKWrapper::AssImpSceneWrapper* assImpSceneWrapper = azrtti_cast <AssImpSDKWrapper::AssImpSceneWrapper*>(m_sceneWrapper.get());

                AZStd::pair<AssImpSDKWrapper::AssImpSceneWrapper::AxisVector, int32_t> upAxisAndSign = assImpSceneWrapper->GetUpVectorAndSign();

                if (upAxisAndSign.second <= 0)
                {
                    AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Negative scene orientation is not a currently supported orientation.");
                    return false;
                }
                switch (upAxisAndSign.first)
                {
                case AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::X:
                    scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::XUp);
                    break;
                case AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::Y:
                    scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::YUp);
                    break;
                case AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::Z:
                    scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::ZUp);
                    break;
                default:
                    AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unknown scene orientation, %d.", upAxisAndSign.first);
                    AZ_Assert(false, "Unknown scene orientation, %d.", upAxisAndSign.first);
                    break;
                }

                AZStd::queue<SceneBuilder::QueueNode> nodes;
                nodes.emplace(AZStd::move(sceneRoot), scene.GetGraph().GetRoot());
                RenamedNodesMap nodeNameMap;

                while (!nodes.empty())
                {
                    SceneBuilder::QueueNode& node = nodes.front();
                    AZ_Assert(node.m_node, "Empty asset importer node queued");
                    if (!nodeNameMap.RegisterNode(node.m_node, scene.GetGraph(), node.m_parent))
                    {
                        AZ_TracePrintf(Utilities::ErrorWindow, "Failed to register asset importer node in name table.");
                        // Skip this node since it could not be registered
                        nodes.pop();
                        continue;
                    }
                    AZStd::string nodeName = nodeNameMap.GetNodeName(node.m_node);
                    SanitizeNodeName(nodeName);

                    AZ_TraceContext("SceneAPI Node Name", nodeName);
                    Containers::SceneGraph::NodeIndex newNode = scene.GetGraph().AddChild(node.m_parent, nodeName.c_str());

                    AZ_Error(Utilities::ErrorWindow, newNode.IsValid(), "Failed to add Asset Importer node to scene graph");
                    if (!newNode.IsValid())
                    {
                        continue;
                    }

                    AssImpNodeEncounteredContext sourceNodeEncountered(scene, newNode, *assImpSceneWrapper, *m_sceneSystem, nodeNameMap, *azrtti_cast<AZ::AssImpSDKWrapper::AssImpNodeWrapper*>(node.m_node.get()));
                    Events::ProcessingResultCombiner nodeResult;
                    nodeResult += Events::Process(sourceNodeEncountered);

                    // If no importer created data, we still create an empty node that may eventually contain a transform
                    if (sourceNodeEncountered.m_createdData.empty())
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Success,
                            "Importers returned success but no data was created");
                        AZStd::shared_ptr<DataTypes::IGraphObject> nullData(nullptr);
                        sourceNodeEncountered.m_createdData.emplace_back(nullData);
                        nodeResult += Events::ProcessingResult::Success;
                    }

                    // Create single node since only one piece of graph data was created
                    if (sourceNodeEncountered.m_createdData.size() == 1)
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Ignored,
                            "An importer created data, but did not return success");
                        if (nodeResult.GetResult() == Events::ProcessingResult::Failure)
                        {
                            AZ_TracePrintf(Utilities::ErrorWindow, "One or more importers failed to create data.");
                        }

                        AssImpSceneDataPopulatedContext dataProcessed(sourceNodeEncountered,
                            sourceNodeEncountered.m_createdData[0], nodeName.c_str());
                        Events::ProcessingResult result = AddDataNodeWithContexts(dataProcessed);
                        if (result != Events::ProcessingResult::Failure)
                        {
                            newNode = dataProcessed.m_currentGraphPosition;
                        }
                    }
                    // Create an empty parent node and place all data under it. The remaining
                    // tree will be built off of this as the logical parent
                    else
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Ignored,
                            "%i importers created data, but did not return success",
                            sourceNodeEncountered.m_createdData.size());
                        if (nodeResult.GetResult() == Events::ProcessingResult::Failure)
                        {
                            AZ_TracePrintf(Utilities::ErrorWindow, "One or more importers failed to create data.");
                        }

                        size_t offset = nodeName.length();
                        for (size_t i = 0; i < sourceNodeEncountered.m_createdData.size(); ++i)
                        {
                            nodeName += '_';
                            nodeName += AZStd::to_string(aznumeric_cast<AZ::u64>(i + 1));

                            Containers::SceneGraph::NodeIndex subNode =
                                scene.GetGraph().AddChild(newNode, nodeName.c_str());
                            AZ_Assert(subNode.IsValid(), "Failed to create new scene sub node");
                            AssImpSceneDataPopulatedContext dataProcessed(sourceNodeEncountered,
                                sourceNodeEncountered.m_createdData[i], nodeName);
                            dataProcessed.m_currentGraphPosition = subNode;
                            AddDataNodeWithContexts(dataProcessed);

                            // Remove the temporary extension again.
                            nodeName.erase(offset, nodeName.length() - offset);
                        }
                    }

                    AZ_Assert(nodeResult.GetResult() == Events::ProcessingResult::Success,
                        "No importers successfully added processed scene data.");
                    AZ_Assert(newNode != node.m_parent,
                        "Failed to update current graph position during data processing.");

                    int childCount = node.m_node->GetChildCount();
                    for (int i = 0; i < childCount; ++i)
                    {
                        const std::shared_ptr<SDKNode::NodeWrapper> nodeWrapper = node.m_node->GetChild(i);
                        auto assImpNodeWrapper = azrtti_cast<AssImpSDKWrapper::AssImpNodeWrapper*>(nodeWrapper.get());

                        AZ_Assert(assImpNodeWrapper, "Child node is not the expected AssImpNodeWrapper type");

                        std::shared_ptr<AssImpSDKWrapper::AssImpNodeWrapper> child = std::make_shared<AssImpSDKWrapper::AssImpNodeWrapper>(assImpNodeWrapper->GetAssImpNode());
                        if (child)
                        {
                            nodes.emplace(AZStd::move(child), newNode);
                        }
                    }

                    nodes.pop();
                };

                Events::ProcessingResult result = Events::Process<AssImpFinalizeSceneContext>(scene, *assImpSceneWrapper, *m_sceneSystem, nodeNameMap);
                if (result == Events::ProcessingResult::Failure)
                {
                    return false;
                }

                return true;
            }

            void SceneImporter::SanitizeNodeName(AZStd::string& nodeName) const
            {
                // Replace % with something else so it is safe for use in printfs.
                AZStd::replace(nodeName.begin(), nodeName.end(), '%', '_');
            }

        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
