/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BoxShape.h"

#include <AzCore/Math/Color.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_unordered_set.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeDisplay.h>
#include <random>

namespace LmbrCentral
{
    BoxShape::BoxShape()
        : m_nonUniformScaleChangedHandler([this](const AZ::Vector3& scale) {this->OnNonUniformScaleChanged(scale); })
    {
    }

    void BoxShape::Reflect(AZ::ReflectContext* context)
    {
        BoxShapeConfig::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoxShape>()
                ->Version(1)
                ->Field("Configuration", &BoxShape::m_boxShapeConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BoxShape>("Box Shape", "Box shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BoxShape::m_boxShapeConfig, "Box Configuration", "Box shape configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
            }
        }
    }

    void BoxShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_currentNonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_currentNonUniformScale, m_entityId, &AZ::NonUniformScaleRequests::GetScale);
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        BoxShapeComponentRequestsBus::Handler::BusConnect(m_entityId);

        AZ::NonUniformScaleRequestBus::Event(m_entityId, &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent,
            m_nonUniformScaleChangedHandler);
    }

    void BoxShape::Deactivate()
    {
        m_nonUniformScaleChangedHandler.Disconnect();
        BoxShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void BoxShape::InvalidateCache(InvalidateShapeCacheReason reason)
    {
        m_intersectionDataCache.InvalidateCache(reason);
    }

    void BoxShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::TransformChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    void BoxShape::OnNonUniformScaleChanged(const AZ::Vector3& scale)
    {
        m_currentNonUniformScale = scale;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void BoxShape::SetBoxDimensions(const AZ::Vector3& dimensions)
    {
        m_boxShapeConfig.m_dimensions = dimensions;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    AZ::Aabb BoxShape::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig, m_currentNonUniformScale);
        return m_intersectionDataCache.m_aabb;
    }

    void BoxShape::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds)
    {
        const AZ::Vector3 extent(m_boxShapeConfig.m_dimensions * m_currentNonUniformScale * 0.5f);
        bounds = AZ::Aabb::CreateFromMinMax(-extent, extent);
        transform = m_currentTransform;
    }

    bool BoxShape::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig, m_currentNonUniformScale);

        if (m_intersectionDataCache.m_axisAligned)
        {
            return m_intersectionDataCache.m_aabb.Contains(point);
        }

        return m_intersectionDataCache.m_obb.Contains(point);
    }

    float BoxShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig, m_currentNonUniformScale);

        if (m_intersectionDataCache.m_axisAligned)
        {
            return m_intersectionDataCache.m_aabb.GetDistanceSq(point);
        }

        return m_intersectionDataCache.m_obb.GetDistanceSq(point);
    }

    bool BoxShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig, m_currentNonUniformScale);

        if (m_intersectionDataCache.m_axisAligned)
        {
            const float rayLength = 1000.0f;
            AZ::Vector3 scaledDir = dir * rayLength;
            AZ::Vector3 startNormal;
            float end;

            float t;
            const bool intersection = AZ::Intersect::IntersectRayAABB(
                src, scaledDir, scaledDir.GetReciprocal(),
                m_intersectionDataCache.m_aabb, t, end, startNormal) > 0;

            distance = rayLength * t;
            return intersection;
        }

        const bool intersection = AZ::Intersect::IntersectRayObb(src, dir, m_intersectionDataCache.m_obb, distance) > 0;
        return intersection;
    }

    ShapeTriangulation BoxShape::GetShapeTriangulation()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig);
        return m_intersectionDataCache.m_triangulation;
    }

    AZ::Vector3 BoxShape::GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig, m_currentNonUniformScale);

        float x = 0;
        float y = 0;
        float z = 0;

        // Points should be generated just inside the shape boundary
        constexpr float insideMargin = 0.999f;

        AZ::Vector3 boxMin = m_intersectionDataCache.m_scaledDimensions * -0.5f * insideMargin;
        AZ::Vector3 boxMax = m_intersectionDataCache.m_scaledDimensions * 0.5f * insideMargin;

        // As std:normal_distribution requires a std:random_engine to be passed in,
        // As std:normal_distribution requires a std:random_engine to be passed in, create one using a random seed that is guaranteed to be properly
        // random each time it is called
        time_t seedVal;
        seedVal = AZ::Sfmt::GetInstance().Rand64();
        std::default_random_engine generator;
        generator.seed(static_cast<unsigned int>(seedVal));

        switch(randomDistribution)
        {
        case AZ::RandomDistributionType::Normal:
            {
                const float mean = 0.0f; //Mean will always be 0

                //stdDev will be the sqrt of the max value (which is the total variation)
                float stdDev = sqrtf(boxMax.GetX());
                std::normal_distribution<float> normalDist =
                    std::normal_distribution<float>(mean, stdDev);
                x = normalDist(generator);
                //Normal distributions can sometimes produce values outside of our desired range
                //We just need to clamp
                x = AZStd::clamp<float>(x, boxMin.GetX(), boxMax.GetX());

                stdDev = sqrtf(boxMax.GetY());
                normalDist = std::normal_distribution<float>(mean, stdDev);
                y = normalDist(generator);

                y = AZStd::clamp<float>(y, boxMin.GetY(), boxMax.GetY());

                stdDev = sqrtf(boxMax.GetZ());
                normalDist = std::normal_distribution<float>(mean, stdDev);
                z = normalDist(generator);

                z = AZStd::clamp<float>(z, boxMin.GetZ(), boxMax.GetZ());
            }
            break;
        case AZ::RandomDistributionType::UniformReal:
            {
                std::uniform_real_distribution<float> uniformRealDist =
                    std::uniform_real_distribution<float>(boxMin.GetX(), boxMax.GetX());
                x = uniformRealDist(generator);

                uniformRealDist = std::uniform_real_distribution<float>(boxMin.GetY(), boxMax.GetY());
                y = uniformRealDist(generator);

                uniformRealDist = std::uniform_real_distribution<float>(boxMin.GetZ(), boxMax.GetZ());
                z = uniformRealDist(generator);
            }
            break;
        default:
            AZ_Warning("BoxShape", false, "Unsupported random distribution type. Returning default vector (0,0,0)");
            break;
        }

        // transform to world space
        return m_currentTransform.TransformPoint(AZ::Vector3(x, y, z));
    }

    void BoxShape::BoxIntersectionDataCache::UpdateIntersectionParamsImpl(
        const AZ::Transform& currentTransform, const BoxShapeConfig& configuration, const AZ::Vector3& currentNonUniformScale)
    {
        AZ::Transform worldFromLocalNormalized = currentTransform;
        const float entityScale = worldFromLocalNormalized.ExtractUniformScale();

        m_currentPosition = worldFromLocalNormalized.GetTranslation();
        m_scaledDimensions = configuration.m_dimensions * currentNonUniformScale * entityScale;

        AZ::Quaternion worldFromLocalQuaternion = worldFromLocalNormalized.GetRotation();
        if (worldFromLocalQuaternion.IsClose(AZ::Quaternion::CreateIdentity()))
        {
            AZ::Vector3 boxMin = m_scaledDimensions * -0.5f;
            boxMin = worldFromLocalNormalized.TransformPoint(boxMin);

            AZ::Vector3 boxMax = m_scaledDimensions * 0.5f;
            boxMax = worldFromLocalNormalized.TransformPoint(boxMax);

            m_aabb = AZ::Aabb::CreateFromMinMax(boxMin, boxMax);
            m_obb = AZ::Obb::CreateFromAabb(m_aabb);

            m_axisAligned = true;
        }
        else
        {
            const AZ::Vector3 halfLengthVector = m_scaledDimensions * 0.5f;
            m_obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
                m_currentPosition,
                worldFromLocalNormalized.GetRotation(),
                halfLengthVector);

            m_aabb = AZ::Aabb::CreateFromObb(m_obb);
            m_axisAligned = false;
        }

        // Create triangulation
        m_triangulation.triangles.clear();
        auto currentPos = currentTransform.GetTranslation();

        // Compute which dimensions are flat; this allows us to omit trivial triangles
        AZStd::fixed_unordered_set<AZ::u8, 1, 3> flatDimensions;
        for (AZ::u8 dim = 0; dim < 3; dim++) {
            if (AZ::IsClose(configuration.m_dimensions.GetElement(dim), 0.f, FLT_EPSILON)) { flatDimensions.insert(dim); }
        }

        if (flatDimensions.size() <= 1) {
            for (AZ::u8 dim0 = 0; dim0 < 3; dim0++) {
                AZ::u8 dim1 = (dim0 + 1) % 3;
                AZ::u8 dim2 = (dim0 + 2) % 3;

                // Skip this face if any of its extents are zero
                if (flatDimensions.find(dim1) != flatDimensions.end() ||
                    flatDimensions.find(dim2) != flatDimensions.end()
                    )
                { continue; }

                // Get the extents along each basis. 0 is the primary basis; the faces to
                // be added on this iteration have normals parllel to basis 0.
                const auto& extent0 = m_obb.GetAxis(dim0) * m_obb.GetHalfLength(dim0);
                const auto& extent1 = m_obb.GetAxis(dim1) * m_obb.GetHalfLength(dim1);
                const auto& extent2 = m_obb.GetAxis(dim2) * m_obb.GetHalfLength(dim2);

                // The vertices of the faces, not yet offset by the primary basis
                const AZStd::array<AZ::Vector3, 4> faceVertices = {
                    currentPos - extent1 - extent2,
                    currentPos - extent1 + extent2,
                    currentPos + extent1 + extent2,
                    currentPos + extent1 - extent2,
                };

                // Winding orders for triangles (CCW order)
                using TriVertIndices = AZStd::array<AZ::u8, 3>;
                static const AZStd::array<TriVertIndices, 2> triWindings = {
                    TriVertIndices{ 2, 0, 3 },
                    TriVertIndices{ 1, 0, 2 }
                };

                const auto addFace = [&](int off) {
                    for (auto& winding : triWindings) {
                        m_triangulation.triangles.push_back();
                        Triangle& t = m_triangulation.triangles.back();
                        for (auto vertIt = winding.begin(); vertIt != winding.end(); ++vertIt) {
                            auto ix = vertIt - winding.begin();
                            if (off == -1) ix = (winding.array_size-1) - ix; // Reverse winding order for opposite face
                            t.vertices[ix] = faceVertices[*vertIt] + (extent0 * (float)off);
                        }
                    }
                };

                addFace(-1);
                if (flatDimensions.find(dim0) == flatDimensions.end()) {
                    // If this dimension is not flat, add the other face (otherwise they are identical)
                    addFace(1);
                }
            }

        } else {
            // If there are 2 or 3 flat dimensions, there are no triangles (the "box" is a line or a point, respectively)
            // Do nothing
        }
    }

    void DrawBoxShape(
        const ShapeDrawParams& shapeDrawParams, const BoxShapeConfig& boxShapeConfig,
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& nonUniformScale)
    {
        const AZ::Vector3 boxMin = boxShapeConfig.m_dimensions * nonUniformScale * -0.5f;
        const AZ::Vector3 boxMax = boxShapeConfig.m_dimensions * nonUniformScale * 0.5f;

        if (shapeDrawParams.m_filled)
        {
            debugDisplay.SetColor(shapeDrawParams.m_shapeColor.GetAsVector4());
            debugDisplay.DrawSolidBox(boxMin, boxMax);
        }

        debugDisplay.SetColor(shapeDrawParams.m_wireColor.GetAsVector4());
        debugDisplay.DrawWireBox(boxMin, boxMax);
    }
} // namespace LmbrCentral
