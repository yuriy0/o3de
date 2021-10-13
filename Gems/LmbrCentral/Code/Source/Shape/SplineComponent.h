/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>
#include <Rendering/EntityDebugDisplayComponent.h>

namespace LmbrCentral
{
    /// Common functionality and data for the SplineComponent.
    class SplineCommon
    {
    public:
        AZ_CLASS_ALLOCATOR(SplineCommon, AZ::SystemAllocator, 0);
        AZ_RTTI(SplineCommon, "{91A31D7E-F63A-4AA8-BC50-909B37F0AD8B}");

        SplineCommon();
        virtual ~SplineCommon() = default;

        static void Reflect(AZ::ReflectContext* context);

        void ChangeSplineType(AZ::u64 splineType);

        /// Override callbacks to be used when spline changes/is modified.
        void SetCallbacks(
            const AZ::IndexFunction& OnAddVertex, const AZ::IndexFunction& OnRemoveVertex,
            const AZ::IndexFunction& OnUpdateVertex, const AZ::VoidFunction& OnSetVertices,
            const AZ::VoidFunction& OnClearVertices, const AZ::VoidFunction& OnChangeType,
            const AZ::BoolFunction& OnOpenClose);

        AZ::SplinePtr m_spline; ///< Reference to the underlying spline data.

    private:
        AZ::u32 OnChangeSplineType();

        AZ::u64 m_splineType = AZ::LinearSpline::RTTI_Type().GetHash(); ///< The currently set spline type (default to Linear).

        AZ::IndexFunction m_onAddVertex = nullptr;
        AZ::IndexFunction m_onRemoveVertex = nullptr;
        AZ::IndexFunction m_onUpdateVertex = nullptr;
        AZ::VoidFunction m_onSetVertices = nullptr;
        AZ::VoidFunction m_onClearVertices = nullptr;
        AZ::VoidFunction m_onChangeType = nullptr;
        AZ::BoolFunction m_onOpenCloseChange = nullptr;
    };

    /* Holds configuration for spline display and implements drawing of the spline debug display
     */
    class SplineDisplay
    {
    public:
        AZ_CLASS_ALLOCATOR(SplineDisplay, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(SplineDisplay, "{8F9962D2-E9D5-4B36-84F8-B39D52549AC4}");
        static void Reflect(AZ::ReflectContext* context);

        SplineDisplay();

        void Draw(const AZ::Spline& spline, const AZ::Transform& worldFromLocal, AzFramework::DebugDisplayRequests& displayContext, bool drawLabels) const;

        bool showNormals;
        bool showTangents;

    private:
        void Draw(const AZ::Spline& spline, const AZ::Transform& worldFromLocal, size_t begin, size_t end, AzFramework::DebugDisplayRequests& displayContext, bool drawLabels) const;
        static void DrawVertices(
            const AZ::Spline& spline, const AZ::Transform& worldFromLocal,
            const size_t begin, const size_t end, AzFramework::DebugDisplayRequests& debugDisplay);
    };

    /// Component interface to core spline implementation.
    class SplineComponent
        : public AZ::Component
        , private SplineComponentRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        friend class EditorSplineComponent;

        AZ_COMPONENT(SplineComponent, "{F0905297-1E24-4044-BFDA-BDE3583F1E57}");

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // SplineComponentRequestBus
        AZ::SplinePtr GetSpline() override;
        void ChangeSplineType(AZ::u64 splineType) override;
        void SetClosed(bool closed) override;

        // SplineComponentRequestBus/VertexContainerInterface
        bool GetVertex(size_t index, AZ::Vector3& vertex) const override;
        void AddVertex(const AZ::Vector3& vertex) override;
        bool UpdateVertex(size_t index, const AZ::Vector3& vertex) override;
        bool InsertVertex(size_t index, const AZ::Vector3& vertex) override;
        bool RemoveVertex(size_t index) override;
        void SetVertices(const AZStd::vector<AZ::Vector3>& vertices) override;
        void ClearVertices() override;
        size_t Size() const override;
        bool Empty() const override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SplineService", 0x2b674d3c));
            provided.push_back(AZ_CRC("VariableVertexContainerService", 0x70c58740));
            provided.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("SplineService", 0x2b674d3c));
            incompatible.push_back(AZ_CRC("VariableVertexContainerService", 0x70c58740));
            incompatible.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        SplineCommon m_splineCommon; ///< Stores common spline functionality and properties.
        AZ::Transform m_currentTransform; ///< Caches the current transform for the entity on which this component lives.
    };

    class SplineDebugDisplayComponent
        : public EntityDebugDisplayComponent
    {
    public:
        AZ_COMPONENT(SplineDebugDisplayComponent, "{8246DC9A-A8F0-4CBD-9DD2-FF271BE37B2F}", EntityDebugDisplayComponent);

        SplineDebugDisplayComponent() = default;
        SplineDebugDisplayComponent(const SplineDisplay& displ)
            : m_display(displ)
        {}

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("SplineService", 0x2b674d3c));
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ::Crc32("SplineDebugService"));
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ::Crc32("SplineDebugService"));
        }

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // EntityDebugDisplayComponent
        void Draw(AzFramework::DebugDisplayRequests& debugDisplay) override;

        SplineDisplay m_display;
        AZ::SplinePtr m_spline; ///< Reference to the underlying spline data.
    private:
        AZ_DISABLE_COPY_MOVE(SplineDebugDisplayComponent);
    };
} // namespace LmbrCentral
