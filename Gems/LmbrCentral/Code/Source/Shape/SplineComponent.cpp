/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SplineComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Viewport/ViewportColors.h>

namespace LmbrCentral
{
    using SplineComboBoxVec = AZStd::vector<AZStd::pair<size_t, AZStd::string>>;
    static SplineComboBoxVec PopulateSplineTypeList()
    {
        return SplineComboBoxVec
        {
            AZStd::make_pair(AZ::LinearSpline::RTTI_Type().GetHash(), "Linear"),
            AZStd::make_pair(AZ::BezierSpline::RTTI_Type().GetHash(), "Bezier"),
            AZStd::make_pair(AZ::CatmullRomSpline::RTTI_Type().GetHash(), "Catmull-Rom")
        };
    }

    static AZ::SplinePtr MakeSplinePtr(AZ::u64 splineType)
    {
        if (splineType == AZ::LinearSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::LinearSpline>();
        }

        if (splineType == AZ::BezierSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::BezierSpline>();
        }

        if (splineType == AZ::CatmullRomSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::CatmullRomSpline>();
        }

        AZ_Assert(false, "Unhandled spline type %d in %s", splineType, __FUNCTION__);

        return nullptr;
    }

    static AZ::SplinePtr CopySplinePtr(AZ::u64 splineType, const AZ::SplinePtr& spline)
    {
        if (splineType == AZ::LinearSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::LinearSpline>(*spline);
        }

        if (splineType == AZ::BezierSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::BezierSpline>(*spline);
        }

        if (splineType == AZ::CatmullRomSpline::RTTI_Type().GetHash())
        {
            return AZStd::make_shared<AZ::CatmullRomSpline>(*spline);
        }

        AZ_Assert(false, "Unhandled spline type %d in %s", splineType, __FUNCTION__);

        return nullptr;
    }

    SplineCommon::SplineCommon()
    {
        m_spline = MakeSplinePtr(m_splineType);
    }

    void SplineCommon::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SplineCommon>()
                ->Version(1)
                ->Field("Spline Type", &SplineCommon::m_splineType)
                ->Field("Spline", &SplineCommon::m_spline);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SplineCommon>("Configuration", "Spline configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        //->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly) // disabled - prevents ChangeNotify attribute firing correctly
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SplineCommon::m_splineType, "Spline Type", "Interpolation type to use between vertices.")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &PopulateSplineTypeList)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SplineCommon::OnChangeSplineType)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SplineCommon::m_spline, "Spline", "Data representing the spline.")
                        //->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly) // disabled - prevents ChangeNotify attribute firing correctly
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void SplineCommon::ChangeSplineType(AZ::u64 splineType)
    {
        m_splineType = splineType;
        OnChangeSplineType();
    }

    void SplineCommon::SetCallbacks(
        const AZ::IndexFunction& OnAddVertex, const AZ::IndexFunction& OnRemoveVertex,
        const AZ::IndexFunction& OnUpdateVertex, const AZ::VoidFunction& OnSetVertices,
        const AZ::VoidFunction& OnClearVertices, const AZ::VoidFunction& OnChangeType,
        const AZ::BoolFunction& OnOpenClose)
    {
        m_onAddVertex = OnAddVertex;
        m_onRemoveVertex = OnRemoveVertex;
        m_onUpdateVertex = OnUpdateVertex;
        m_onSetVertices = OnSetVertices;
        m_onClearVertices = OnClearVertices;

        m_onChangeType = OnChangeType;
        m_onOpenCloseChange = OnOpenClose;

        m_spline->SetCallbacks(
            OnAddVertex, OnRemoveVertex,
            OnUpdateVertex, OnSetVertices,
            OnClearVertices, OnOpenClose);
    }

    AZ::u32 SplineCommon::OnChangeSplineType()
    {
        AZ::u32 ret = AZ::Edit::PropertyRefreshLevels::None;

        if (m_spline->RTTI_GetType().GetHash() != m_splineType)
        {
            m_spline = CopySplinePtr(m_splineType, m_spline);
            m_spline->SetCallbacks(
                m_onAddVertex, m_onRemoveVertex, m_onUpdateVertex,
                m_onSetVertices, m_onClearVertices, m_onOpenCloseChange);

            ret = AZ::Edit::PropertyRefreshLevels::EntireTree;

            if (m_onChangeType)
            {
                m_onChangeType();
            }
        }

        return ret;
    }

    /// BehaviorContext forwarder for SplineComponentNotificationBus
    class BehaviorSplineComponentNotificationBusHandler
        : public SplineComponentNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorSplineComponentNotificationBusHandler, "{05816EA4-A4F0-4FB4-A82B-D6537B215D25}", AZ::SystemAllocator, OnSplineChanged);

        void OnSplineChanged() override
        {
            Call(FN_OnSplineChanged);
        }
    };

    void SplineComponent::Reflect(AZ::ReflectContext* context)
    {
        SplineCommon::Reflect(context);
        SplineDisplay::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SplineComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &SplineComponent::m_splineCommon);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SplineComponentNotificationBus>("SplineComponentNotificationBus")->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)->
                Handler<BehaviorSplineComponentNotificationBusHandler>();

            behaviorContext->EBus<SplineComponentRequestBus>("SplineComponentRequestBus")
                //->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                ->Attribute(AZ::Script::Attributes::Module, "shape")
                ->Event("GetSpline", &SplineComponentRequestBus::Events::GetSpline)
                ->Event("SetClosed", &SplineComponentRequestBus::Events::SetClosed)
                ->Event("AddVertex", &SplineComponentRequestBus::Events::AddVertex)
                ->Event("UpdateVertex", &SplineComponentRequestBus::Events::UpdateVertex)
                ->Event("InsertVertex", &SplineComponentRequestBus::Events::InsertVertex)
                ->Event("RemoveVertex", &SplineComponentRequestBus::Events::RemoveVertex)
                ->Event("ClearVertices", &SplineComponentRequestBus::Events::ClearVertices);
        }
    }

    void SplineComponent::Activate()
    {
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        SplineComponentRequestBus::Handler::BusConnect(GetEntityId());

        const auto splineChanged = [this]()
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
        };

        const auto vertexAdded = [this, splineChanged](size_t index)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVertexAdded, index);

            splineChanged();
        };

        const auto vertexRemoved = [this, splineChanged](size_t index)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVertexRemoved, index);

            splineChanged();
        };

        const auto vertexUpdated = [this, splineChanged](size_t index)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVertexUpdated, index);

            splineChanged();
        };

        const auto verticesSet = [this, splineChanged]()
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(),
                &SplineComponentNotificationBus::Events::OnVerticesSet,
                m_splineCommon.m_spline->GetVertices()
            );

            splineChanged();
        };

        const auto verticesCleared = [this, splineChanged]()
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVerticesCleared);

            splineChanged();
        };

        const auto openCloseChanged = [this, splineChanged](const bool closed)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnOpenCloseChanged, closed);

            splineChanged();
        };

        m_splineCommon.SetCallbacks(
            vertexAdded,
            vertexRemoved,
            vertexUpdated,
            verticesSet,
            verticesCleared,
            splineChanged,
            openCloseChanged);
    }

    void SplineComponent::Deactivate()
    {
        SplineComponentRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void SplineComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
    }

    AZ::SplinePtr SplineComponent::GetSpline()
    {
        return m_splineCommon.m_spline;
    }

    void SplineComponent::ChangeSplineType(AZ::u64 splineType)
    {
        m_splineCommon.ChangeSplineType(splineType);
    }

    bool SplineComponent::UpdateVertex(size_t index, const AZ::Vector3& vertex)
    {
        return m_splineCommon.m_spline->m_vertexContainer.UpdateVertex(index, vertex);
    }

    bool SplineComponent::GetVertex(size_t index, AZ::Vector3& vertex) const
    {
        return m_splineCommon.m_spline->m_vertexContainer.GetVertex(index, vertex);
    }

    void SplineComponent::AddVertex(const AZ::Vector3& vertex)
    {
        m_splineCommon.m_spline->m_vertexContainer.AddVertex(vertex);
    }

    bool SplineComponent::InsertVertex(size_t index, const AZ::Vector3& vertex)
    {
        return m_splineCommon.m_spline->m_vertexContainer.InsertVertex(index, vertex);
    }

    bool SplineComponent::RemoveVertex(size_t index)
    {
        return m_splineCommon.m_spline->m_vertexContainer.RemoveVertex(index);
    }

    void SplineComponent::SetVertices(const AZStd::vector<AZ::Vector3>& vertices)
    {
        m_splineCommon.m_spline->m_vertexContainer.SetVertices(vertices);
    }

    void SplineComponent::ClearVertices()
    {
        m_splineCommon.m_spline->m_vertexContainer.Clear();
    }

    bool SplineComponent::Empty() const
    {
        return m_splineCommon.m_spline->m_vertexContainer.Empty();
    }

    size_t SplineComponent::Size() const
    {
        return m_splineCommon.m_spline->m_vertexContainer.Size();
    }

    void SplineComponent::SetClosed(const bool closed)
    {
        // set closed callback calls OnSplineChanged
        m_splineCommon.m_spline->SetClosed(closed);
    }

    ///////////////////////////////////////////////////
    // SplineDebugDisplayComponent
    ///////////////////////////////////////////////////
    void SplineDebugDisplayComponent::Reflect(AZ::ReflectContext * context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SplineDebugDisplayComponent, EntityDebugDisplayComponent>()
                ->Version(1)
                ->Field("Spline", &SplineDebugDisplayComponent::m_spline)
                ->Field("Display", &SplineDebugDisplayComponent::m_display)
                ;
        }
    }

    void SplineDebugDisplayComponent::Activate() {
        EBUS_EVENT_ID_RESULT(m_spline, GetEntityId(), SplineComponentRequestBus, GetSpline);

        EntityDebugDisplayComponent::Activate();
    }

    void SplineDebugDisplayComponent::Deactivate() {
        EntityDebugDisplayComponent::Deactivate();
    }

    void SplineDebugDisplayComponent::Draw(AzFramework::DebugDisplayRequests& displayContext) {
        displayContext.SetColor(AzFramework::ViewportColors::SelectedColor);
        m_display.Draw(*m_spline.get(), GetCurrentTransform(), displayContext, true);
    }

    ///////////////////////////////////////////////////
    // SplineDisplay
    ///////////////////////////////////////////////////
    void SplineDisplay::DrawVertices(
        const AZ::Spline& spline, const AZ::Transform& worldFromLocal,
        const size_t begin, const size_t end, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        static const auto offset = AZ::Vector3(0.f, 0.f, -0.1f);
        static const auto vertexColor = AZ::Color(1.f, 0.f, 0.f, 0.65f);
        static const auto labelColor = AZ::Color(1.f, 1.f, 1.f, 1.f);
        debugDisplay.SetColor(vertexColor);

        for (auto i = begin-1; i < end; i++) {
            auto vertexPos = spline.GetVertex(i);

            AZStd::string indexFormat = AZStd::string::format("[%d]", i);
            debugDisplay.SetColor(labelColor);
            debugDisplay.DrawTextLabel(worldFromLocal.TransformPoint(vertexPos + offset), 1.5f, indexFormat.c_str(), true);
            debugDisplay.SetColor(vertexColor);

            debugDisplay.DrawBall(vertexPos, 0.125f);
        }
    }

    void SplineDisplay::Draw(const AZ::Spline& spline, const AZ::Transform& worldFromLocal, size_t begin, size_t end, AzFramework::DebugDisplayRequests& displayContext, bool drawLabels) const
    {
        const size_t granularity = spline.GetSegmentGranularity();
        const auto GetAddr = [&](size_t i, size_t j) {
            return AZ::SplineAddress(i - 1, j / static_cast<float>(granularity));
        };

        AZStd::vector<AZStd::function<void(const AZ::Vector3&, const AZ::SplineAddress&)>> DrawAux;
        if (showNormals) {
            DrawAux.push_back([&](const AZ::Vector3& orig, const AZ::SplineAddress& addr) {
                auto normal = spline.GetNormal(addr);
                displayContext.DrawLine(orig - normal, orig + normal);
            });
        }
        if (showTangents) {
            DrawAux.push_back([&](const AZ::Vector3& orig, const AZ::SplineAddress& addr) {
                auto tangent = spline.GetTangent(addr);
                displayContext.DrawLine(orig-tangent, orig+tangent);
            });
        }
        const auto DrawAuxillaryData = [&](const AZ::Vector3& orig, const AZ::SplineAddress& addr) {
            for (auto& fn : DrawAux) { fn(orig, addr); };
        };

        for (size_t i = begin; i < end; ++i)
        {
            AZ::Vector3 p1 = spline.GetVertex(i - 1);
            DrawAuxillaryData(p1, GetAddr(i, 0));

            for (size_t j = 1; j <= granularity; ++j)
            {
                auto addr = GetAddr(i, j);
                AZ::Vector3 p2 = spline.GetPosition(addr);
                displayContext.DrawLine(p1, p2);
                DrawAuxillaryData(p2, addr);
                p1 = p2;
            }
        }

        if (drawLabels) DrawVertices(spline, worldFromLocal, begin, end, displayContext);
    }

    void SplineDisplay::Draw(const AZ::Spline& spline, const AZ::Transform& worldFromLocal, AzFramework::DebugDisplayRequests& displayContext, bool drawLabels) const {
        const size_t vertexCount = spline.GetVertexCount();
        if (vertexCount == 0)
        {
            return;
        }

        // render spline
        if (spline.RTTI_IsTypeOf(AZ::LinearSpline::RTTI_Type()) || spline.RTTI_IsTypeOf(AZ::BezierSpline::RTTI_Type()))
        {
            Draw(spline, worldFromLocal, 1, spline.IsClosed() ? vertexCount + 1 : vertexCount, displayContext, drawLabels);
        }
        else if (spline.RTTI_IsTypeOf(AZ::CatmullRomSpline::RTTI_Type()))
        {
            // catmull-rom splines use the first and last points as control points only, omit those for display
            Draw(spline, worldFromLocal, spline.IsClosed() ? 1 : 2, spline.IsClosed() ? vertexCount + 1 : vertexCount - 1, displayContext, drawLabels);
        }
    }

    SplineDisplay::SplineDisplay()
        : showNormals(false)
        , showTangents(false)
    {}

    void SplineDisplay::Reflect(AZ::ReflectContext * context) {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SplineDisplay>()
                ->Version(1)
                ->Field("showNormals", &SplineDisplay::showNormals)
                ->Field("showTangents", &SplineDisplay::showTangents)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SplineDisplay>(
                    "Spline Display", "Describes how to display a spline")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SplineDisplay::showNormals, "Show normals?", "")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SplineDisplay::showTangents, "Show tangents?", "")
                    ;
            }
        }
    }
} // namespace LmbrCentral
