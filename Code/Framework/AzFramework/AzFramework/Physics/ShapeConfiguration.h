/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Physics
{
    /// Used to identify shape configuration type from base class.
    enum class ShapeType : AZ::u8
    {
        Sphere,
        Box,
        Capsule,
        Cylinder,
        ConvexHull, ///< Not Supported in physx
        TriangleMesh, ///< Not Supported in physx
        Native, ///< Native shape configuration if user wishes to bypass generic shape configurations.
        PhysicsAsset, ///< Shapes configured in the asset.
        CookedMesh, ///< Stores a blob of mesh data cooked for the specific engine.
    };

    class ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(ShapeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(ShapeConfiguration, "{1FD56C72-6055-4B35-9253-07D432B94E91}");
        static void Reflect(AZ::ReflectContext* context);
        virtual ~ShapeConfiguration() = default;
        virtual ShapeType GetShapeType() const = 0;

        AZ::Vector3 m_scale = AZ::Vector3::CreateOne();
    };

    class SphereShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(SphereShapeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(SphereShapeConfiguration, "{0B9F3D2E-0780-4B0B-BFEE-B41C5FDE774A}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);
        explicit SphereShapeConfiguration(float radius = 0.5f);

        ShapeType GetShapeType() const override { return ShapeType::Sphere; }

        float m_radius = 0.5f;
    };

    class BoxShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(BoxShapeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(BoxShapeConfiguration, "{E58040ED-3E50-4882-B0E9-525E7A548F8D}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);
        explicit BoxShapeConfiguration(const AZ::Vector3& boxDimensions = AZ::Vector3::CreateOne());

        ShapeType GetShapeType() const override { return ShapeType::Box; }

        AZ::Vector3 m_dimensions = AZ::Vector3::CreateOne();
    };

    class CapsuleShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(CapsuleShapeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(CapsuleShapeConfiguration, "{19C6A07E-5644-46B7-A49E-48703B56ED32}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);
        explicit CapsuleShapeConfiguration(float height = 1.0f, float radius = 0.25f);

        ShapeType GetShapeType() const override { return ShapeType::Capsule; }

        float m_height = 1.0f;
        float m_radius = 0.25f;

    private:
        void OnHeightChanged();
        void OnRadiusChanged();
    };

    class ConvexHullShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(ConvexHullShapeConfiguration, AZ::SystemAllocator, 0);

        ShapeType GetShapeType() const override { return ShapeType::ConvexHull; }

        const void* m_vertexData = nullptr;
        AZ::u32 m_vertexCount = 0;
        AZ::u32 m_vertexStride = 4;

        const void* m_planeData = nullptr;
        AZ::u32 m_planeCount = 0;
        AZ::u32 m_planeStride = 4;

        const void* m_adjacencyData = nullptr;
        AZ::u32 m_adjacencyCount = 0;
        AZ::u32 m_adjacencyStride = 4;

        bool m_copyData = true; ///< If set, vertex buffer will be copied in the native physics implementation,
    };

    class TriangleMeshShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(TriangleMeshShapeConfiguration, AZ::SystemAllocator, 0);

        ShapeType GetShapeType() const override { return ShapeType::TriangleMesh; }

        const void* m_vertexData = nullptr;
        AZ::u32 m_vertexCount = 0;
        AZ::u32 m_vertexStride = 4; ///< Data size of a given vertex, e.g. float * 3 = 12.

        const void* m_indexData = nullptr;
        AZ::u32 m_indexCount = 0;
        AZ::u32 m_indexStride = 12; ///< Data size of indices for a given triangle, e.g. AZ::u32 * 3 = 12.

        bool m_copyData = true; ///< If set, vertex/index buffers will be copied in the native physics implementation,
                                ///< and don't need to be kept alive by the caller;
    };

    class PhysicsAssetShapeConfiguration 
        : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysicsAssetShapeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(PhysicsAssetShapeConfiguration, "{1C0046D9-BC9E-4F93-9F0E-D62654FB18EA}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);
        ShapeType GetShapeType() const override;

        AZ::Data::Asset<AZ::Data::AssetData> m_asset{ AZ::Data::AssetLoadBehavior::PreLoad };
        AZ::Vector3 m_assetScale = AZ::Vector3::CreateOne();
        bool m_useMaterialsFromAsset = true;
        AZ::u8 m_subdivisionLevel = 4; ///< The level of subdivision if a primitive shape is replaced with a convex mesh due to scaling.
    };

    class NativeShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeShapeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(NativeShapeConfiguration, "{6CB8FE4A-A577-49AF-81F4-4F1AD245859A}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        ShapeType GetShapeType() const override { return ShapeType::Native; }

        void* m_nativeShapePtr = nullptr; ///< Native shape ptr. This will not be serialised
        AZ::Vector3 m_nativeShapeScale = AZ::Vector3::CreateOne(); ///< Native shape scale. This will be serialised
    };

    class CookedMeshShapeConfiguration 
        : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(CookedMeshShapeConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(CookedMeshShapeConfiguration, "{D9E58241-36BB-4A4F-B50C-1736EB7E841F}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        enum class MeshType : AZ::u8
        {
            TriangleMesh = 0,
            Convex
        };
        
        CookedMeshShapeConfiguration() = default;
        CookedMeshShapeConfiguration(const CookedMeshShapeConfiguration&);
        CookedMeshShapeConfiguration& operator=(const CookedMeshShapeConfiguration&);
        ~CookedMeshShapeConfiguration();

        ShapeType GetShapeType() const override;

        //! Sets the cooked data. This will release the cached mesh.
        //! Input data has to be in the physics engine specific format.
        //! (e.g. in PhysX: result of cookTriangleMesh or cookConvexMesh).
        void SetCookedMeshData(const AZ::u8* cookedData, size_t cookedDataSize, MeshType type);
        const AZStd::vector<AZ::u8>& GetCookedMeshData() const;
        
        MeshType GetMeshType() const;

        void* GetCachedNativeMesh() const;
        void SetCachedNativeMesh(void* cachedNativeMesh) const;

    private:
        void ReleaseCachedNativeMesh();

        AZStd::vector<AZ::u8> m_cookedData;
        MeshType m_type = MeshType::TriangleMesh;
        
        //! Cached native mesh object (e.g. PxConvexMesh or PxTriangleMesh). This data is not serialized.
        mutable void* m_cachedNativeMesh = nullptr;
    };

} // namespace Physics
