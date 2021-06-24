/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/ComponentBus.h>

#include <AzFramework/Viewport/ViewportColors.h>

namespace AZ
{
    class BehaviorContext;
    enum class RandomDistributionType : AZ::u32;
}
namespace LmbrCentral
{
    /// Reason shape cache should be recalculated.
    enum class InvalidateShapeCacheReason
    {
        TransformChange, ///< The cache is invalid because the transform of the entity changed.
        ShapeChange ///< The cache is invalid because the shape configuration/properties changed.
    };

    /// Wrapper for cache of data used for intersection tests
    template <typename ShapeConfiguration>
    class IntersectionTestDataCache
    {
    public:
        virtual ~IntersectionTestDataCache() = default;

        /// @brief Updates the intersection data cache to reflect the current state of the shape.
        /// @param currentTransform The current Transform of the entity.
        /// @param configuration The specific configuration of a shape.
        /// @param currentNonUniformScale (Optional) The current non-uniform scale of the entity (if supported by the shape).
        void UpdateIntersectionParams(
            const AZ::Transform& currentTransform, const ShapeConfiguration& configuration,
            [[maybe_unused]] const AZ::Vector3& currentNonUniformScale = AZ::Vector3::CreateOne())
        {
            // does the cache need updating
            if (m_cacheStatus > ShapeCacheStatus::Current)
            {
                UpdateIntersectionParamsImpl(currentTransform, configuration, currentNonUniformScale); // shape specific cache update
                m_cacheStatus = ShapeCacheStatus::Current; // mark cache as up to date
            }
        }

        /// Mark the cache as needing to be updated.
        void InvalidateCache(const InvalidateShapeCacheReason reason)
        {
            switch (reason)
            {
            case InvalidateShapeCacheReason::TransformChange:
                if (m_cacheStatus < ShapeCacheStatus::ObsoleteTransformChange)
                {
                    m_cacheStatus = ShapeCacheStatus::ObsoleteTransformChange;
                }
                break;
            case InvalidateShapeCacheReason::ShapeChange:
                if (m_cacheStatus < ShapeCacheStatus::ObsoleteShapeChange)
                {
                    m_cacheStatus = ShapeCacheStatus::ObsoleteShapeChange;
                }
                break;
            default:
                break;
            }
        }

    protected:
        /// Derived shape specific implementation of cache update (called from UpdateIntersectionParams).
        virtual void UpdateIntersectionParamsImpl(
            const AZ::Transform& currentTransform, const ShapeConfiguration& configuration,
            const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne()) = 0;

        /// State of shape cache - should the internal shape cache be recalculated, or is it up to date.
        enum class ShapeCacheStatus
        {
            Current, ///< Cache is up to date.
            ObsoleteTransformChange, ///< The cache is invalid because the transform of the entity changed.
            ObsoleteShapeChange ///< The cache is invalid because the shape configuration/properties changed.
        };

        /// Expose read only cache status to derived IntersectionTestDataCache if different
        /// logic want to hapoen based on the cache status (shape/transform).
        ShapeCacheStatus CacheStatus() const
        {
            return m_cacheStatus;
        }

    private:
        ShapeCacheStatus m_cacheStatus = ShapeCacheStatus::Current; ///< The current state of the shape cache.
    };

    struct ShapeComponentGeneric
    {
        static void Reflect(AZ::ReflectContext* context);
    };

    struct Triangle
    {
        AZ_TYPE_INFO(Triangle, "{79D5CB43-DACE-4353-85A1-9DE757F94248}");
        Triangle()
        {
            vertices.fill(AZ::Vector3());
        }
        Triangle(const AZ::Vector3& v0, const AZ::Vector3& v1, const AZ::Vector3& v2)
        {
            vertices[0] = v0;
            vertices[1] = v1;
            vertices[2] = v2;
        }
        AZStd::array<AZ::Vector3, 3> vertices;
    };

    /*
        A shape triangulation is the surface of the shape broken up into triangles (approximately or exactly); no guarantee is made
        about which triangulation this is, but implementations should endeavor to use the fewest number of triangles possible (i.e. most
        efficient triangulation possible).

        The interface contains some metadata about the triangulation (currently, only whether it is exact or not)
        and the actual triangles.
    */
    struct ShapeTriangulation
    {
        AZ_TYPE_INFO(ShapeTriangulation, "{96DEB5BD-DE14-4B31-A6D1-496041F51B51}");
        ShapeTriangulation()
            : isExact(false)
        {}

        bool IsValid() const { return !triangles.empty(); }

        ///< If the shape only has planar faces, i.e. it is equalivent to some triangular mesh or meshes, then this member is 'true'
        ///< and the triangles returned by 'GetTriangles' are one of those equivalent triangular meshes.
        bool isExact;

        ///< The triangles of which this shape is composed.
        AZStd::vector<Triangle> triangles;
    };

    /// Services provided by the Shape Component
    class ShapeComponentRequests : public AZ::ComponentBus
    {
    public:
        /// allows multiple threads to call shape requests
        using MutexType = AZStd::recursive_mutex;

        /// @brief Returns the type of shape that this component holds
        /// @return Crc32 indicating the type of shape
        virtual AZ::Crc32 GetShapeType() = 0;

        /// @brief Returns an AABB that encompasses this entire shape
        /// @return AABB that encompasses the shape
        virtual AZ::Aabb GetEncompassingAabb() = 0;

        /**
        * @brief Returns the local space bounds of a shape and its world transform
        * @param transform AZ::Transform outparam containing the shape transform
        * @param bounds AZ::Aabb outparam containing an untransformed tight fitting bounding box according to the shape parameters
        */
        virtual void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) = 0;

        /// @brief Checks if a given point is inside a shape or outside it
        /// @param point Vector3 indicating the point to be tested
        /// @return bool indicating whether the point is inside or out
        virtual bool IsPointInside(const AZ::Vector3& point) = 0;

        /// @brief Returns the min distance a given point is from the shape
        /// @param point Vector3 indicating point to calculate distance from
        /// @return float indicating distance point is from shape
        virtual float DistanceFromPoint(const AZ::Vector3& point)
        {
            return sqrtf(DistanceSquaredFromPoint(point));
        }

        /// @brief Returns the min squared distance a given point is from the shape
        /// @param point Vector3 indicating point to calculate square distance from
        /// @return float indicating square distance point is from shape
        virtual float DistanceSquaredFromPoint(const AZ::Vector3& point) = 0;

        /// @brief Returns a random position inside the volume.
        /// @param randomDistribution An enum representing the different random distributions to use.
        virtual AZ::Vector3 GenerateRandomPointInside(AZ::RandomDistributionType /*randomDistribution*/)
        {
            AZ_Warning("ShapeComponentRequests", false, "GenerateRandomPointInside not implemented");
            return AZ::Vector3::CreateZero();
        }

        /// @brief Returns if a ray is intersecting the shape.
        virtual bool IntersectRay(const AZ::Vector3& /*src*/, const AZ::Vector3& /*dir*/, float& /*distance*/)
        {
            AZ_Warning("ShapeComponentRequests", false, "IntersectRay not implemented");
            return false;
        }

        /**
        * @brief Returns the triangulation of the shape.
        */
        virtual ShapeTriangulation GetShapeTriangulation()
        {
            return ShapeTriangulation();
        }

        virtual ~ShapeComponentRequests() = default;
    };

    // Bus to service the Shape component requests event group
    using ShapeComponentRequestsBus = AZ::EBus<ShapeComponentRequests>;

    /// Notifications sent by the shape component.
    class ShapeComponentNotifications : public AZ::ComponentBus
    {
    public:
        //! allows multiple threads to call shape requests
        using MutexType = AZStd::recursive_mutex;

        enum class ShapeChangeReasons
        {
            TransformChanged,
            ShapeChanged
        };

        /// @brief Informs listeners that the shape component has been updated (The shape was modified)
        /// @param changeReason Informs listeners of the reason for this shape change (transform change, the shape dimensions being altered)
        virtual void OnShapeChanged(ShapeChangeReasons changeReason) = 0;
    };

    // Bus to service Shape component notifications event group
    using ShapeComponentNotificationsBus = AZ::EBus<ShapeComponentNotifications>;

    /**
    * Common properties of how shape debug drawing can be rendererd.
    */
    struct ShapeDrawParams
    {
        AZ::Color m_shapeColor; ///< Color of underlying shape.
        AZ::Color m_wireColor; ///< Color of wireframe edges of shapes.
        bool m_filled; ///< Whether the shape should be rendered filled, or wireframe only.
    };

    class ShapeComponentConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ShapeComponentConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(ShapeComponentConfig, "{32683353-0EF5-4FBC-ACA7-E220C58F60F5}", AZ::ComponentConfig);

        static void Reflect(AZ::ReflectContext* context);

        ShapeComponentConfig() = default;
        virtual ~ShapeComponentConfig() = default;

        void SetDrawColor(const AZ::Color& drawColor)
        {
            m_drawColor = drawColor;
        }

        const AZ::Color& GetDrawColor() const
        {
            return m_drawColor;
        }

        void SetIsFilled(bool isFilled)
        {
            m_filled = isFilled;
        }

        bool IsFilled() const
        {
            return m_filled;
        }

        ShapeDrawParams GetDrawParams() const
        {
            ShapeDrawParams drawParams;
            drawParams.m_shapeColor = GetDrawColor();
            drawParams.m_wireColor = AzFramework::ViewportColors::WireColor;
            drawParams.m_filled = IsFilled();

            return drawParams;
        }

    private:

        AZ::Color m_drawColor   = AzFramework::ViewportColors::DeselectedColor;
        bool      m_filled      = true;
    };
} // namespace LmbrCentral
