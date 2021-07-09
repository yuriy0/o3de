/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/ImportContexts/ImportContexts.h>

namespace AZ
{
    struct Uuid;

    namespace FbxSDKWrapper
    {
        class FbxNodeWrapper;
        class FbxSceneWrapper;
    }

    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            struct FbxImportContext;

            using CoreScene = Containers::Scene;
            using CoreSceneGraph = Containers::SceneGraph;
            using CoreGraphNodeIndex = Containers::SceneGraph::NodeIndex;
            using CoreProcessingResult = Events::ProcessingResult;

            //inline bool NodeIsOfType(const CoreSceneGraph& graph, CoreGraphNodeIndex nodeIndex, const AZ::Uuid& uuid);
            //inline bool NodeParentIsOfType(const CoreSceneGraph& graph, CoreGraphNodeIndex nodeIndex, 
            //    const AZ::Uuid& uuid);
            //inline bool NodeHasAncestorOfType(const CoreSceneGraph& graph, CoreGraphNodeIndex nodeIndex,
            //    const AZ::Uuid& uuid);
            inline bool IsSkinnedMesh(const FbxSDKWrapper::FbxNodeWrapper& sourceNode);
            CoreProcessingResult AddDataNodeWithContexts(SceneDataPopulatedContextBase& dataContext);
            CoreProcessingResult AddAttributeDataNodeWithContexts(SceneAttributeDataPopulatedContextBase& dataContext);
            bool AreSceneGraphsEqual(const CoreSceneGraph& lhsGraph, const CoreSceneGraph& rhsGraph);
            //inline bool AreScenesEqual(const CoreScene& lhs, const CoreScene& rhs);

            bool IsGraphDataEqual(const AZStd::shared_ptr<const DataTypes::IGraphObject>& lhs,
                const AZStd::shared_ptr<const DataTypes::IGraphObject>& rhs);

            // If the scene contains bindpose information for the node, returns true and sets "xf" to the local transform
            // of the node in bindpose. Returns false if bindpose info is not available for the node.
            bool GetBindPoseLocalTransform(const FbxSDKWrapper::FbxSceneWrapper& sceneWrapper,
                FbxSDKWrapper::FbxNodeWrapper& nodeWrapper, DataTypes::MatrixType& xf);
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ

#include <SceneAPI/SceneBuilder/Importers/FbxImporterUtilities.inl>
