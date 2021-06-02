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
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpTransformImporter.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <assimp/scene.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpImporterUtilities.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            const char* AssImpTransformImporter::s_transformNodeName = "transform";

            AssImpTransformImporter::AssImpTransformImporter()
            {
                BindToCall(&AssImpTransformImporter::ImportTransform);
            }

            void AssImpTransformImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpTransformImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            void EnumBonesInNode2(
                const aiScene* scene, const aiNode* node, AZStd::unordered_map<AZStd::string, const aiNode*>&,
                AZStd::unordered_map<AZStd::string, const aiBone*>& boneLookup)
            {
                /* From AssImp Documentation
                    a) Create a map or a similar container to store which nodes are necessary for the skeleton. Pre-initialise it for all
                   nodes with a "no". b) For each bone in the mesh: b1) Find the corresponding node in the scene's hierarchy by comparing
                   their names. b2) Mark this node as "yes" in the necessityMap. b3) Mark all of its parents the same way until you 1) find
                   the mesh's node or 2) the parent of the mesh's node. c) Recursively iterate over the node hierarchy c1) If the node is
                   marked as necessary, copy it into the skeleton and check its children c2) If the node is marked as not necessary, skip it
                   and do not iterate over its children.
                 */

                for (unsigned meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
                {
                    const aiMesh* mesh = scene->mMeshes[node->mMeshes[meshIndex]];

                    for (unsigned boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
                    {
                        const aiBone* bone = mesh->mBones[boneIndex];

                        boneLookup[bone->mName.C_Str()] = bone;
                    }
                }
            }

            void EnumChildren2(
                const aiScene* scene, const aiNode* node, AZStd::unordered_map<AZStd::string, const aiNode*>& mainBoneList,
                AZStd::unordered_map<AZStd::string, const aiBone*>& boneLookup)
            {
                EnumBonesInNode2(scene, node, mainBoneList, boneLookup);

                for (unsigned childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
                {
                    const aiNode* child = node->mChildren[childIndex];

                    EnumChildren2(scene, child, mainBoneList, boneLookup);
                }
            }
            
            Events::ProcessingResult AssImpTransformImporter::ImportTransform(AssImpSceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "transform");
                const aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();
                
                if (currentNode == scene->mRootNode || IsPivotNode(currentNode->mName))
                {
                    return Events::ProcessingResult::Ignored;
                }

                bool isBone = false;
                AZStd::unordered_map<AZStd::string, const aiNode*> mainBoneList;
                AZStd::unordered_map<AZStd::string, const aiBone*> boneLookup;
                EnumChildren2(scene, scene->mRootNode, mainBoneList, boneLookup);

                if (boneLookup.find(currentNode->mName.C_Str()) != boneLookup.end())
                {
                    isBone = true;
                }

                aiMatrix4x4 combinedTransform = GetConcatenatedLocalTransform(currentNode);

                if (isBone)
                {
                    auto parentNode = currentNode->mParent;

                    aiMatrix4x4 offsetMatrix = boneLookup[currentNode->mName.C_Str()]->mOffsetMatrix;
                    aiMatrix4x4 parentOffset{};

                    if (parentNode && boneLookup.count(parentNode->mName.C_Str()))
                    {
                        auto parentBone = boneLookup[parentNode->mName.C_Str()];

                        parentOffset = parentBone->mOffsetMatrix;
                    }

                    auto inverseOffset = offsetMatrix;
                    inverseOffset.Inverse();

                    auto optionB = parentOffset * inverseOffset;

                    combinedTransform = optionB;
                }

                DataTypes::MatrixType localTransform = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(combinedTransform);
                
                context.m_sourceSceneSystem.SwapTransformForUpAxis(localTransform);
                context.m_sourceSceneSystem.ConvertUnit(localTransform);

                AZStd::shared_ptr<SceneData::GraphData::TransformData> transformData =
                    AZStd::make_shared<SceneData::GraphData::TransformData>(localTransform);
                AZ_Error(SceneAPI::Utilities::ErrorWindow, transformData, "Failed to allocate transform data.");
                if (!transformData)
                {
                    return Events::ProcessingResult::Failure;
                }

                // If it is non-endpoint data populated node, add a transform attribute
                if (context.m_scene.GetGraph().HasNodeContent(context.m_currentGraphPosition))
                {
                    if (!context.m_scene.GetGraph().IsNodeEndPoint(context.m_currentGraphPosition))
                    {
                        AZStd::string nodeName = s_transformNodeName;
                        RenamedNodesMap::SanitizeNodeName(nodeName, context.m_scene.GetGraph(), context.m_currentGraphPosition);
                        AZ_TraceContext("Transform node name", nodeName);

                        Containers::SceneGraph::NodeIndex newIndex =
                            context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, nodeName.c_str());

                        AZ_Error(SceneAPI::Utilities::ErrorWindow, newIndex.IsValid(), "Failed to create SceneGraph node for attribute.");
                        if (!newIndex.IsValid())
                        {
                            return Events::ProcessingResult::Failure;
                        }

                        Events::ProcessingResult transformAttributeResult;
                        AssImpSceneAttributeDataPopulatedContext dataPopulated(context, transformData, newIndex, nodeName);
                        transformAttributeResult = Events::Process(dataPopulated);

                        if (transformAttributeResult != Events::ProcessingResult::Failure)
                        {
                            transformAttributeResult = AddAttributeDataNodeWithContexts(dataPopulated);
                        }

                        return transformAttributeResult;
                    }
                }
                else
                {
                    bool addedData = context.m_scene.GetGraph().SetContent(
                        context.m_currentGraphPosition,
                        transformData);

                    AZ_Error(SceneAPI::Utilities::ErrorWindow, addedData, "Failed to add node data");
                    return addedData ? Events::ProcessingResult::Success : Events::ProcessingResult::Failure;
                }

                return Events::ProcessingResult::Ignored;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
