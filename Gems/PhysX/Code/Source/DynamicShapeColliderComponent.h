#pragma once

#include <ShapeColliderComponent.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <Editor/PolygonPrismMeshUtils.h>
#include <Source/ShapeType.h>

namespace PhysX
{
    using ShapeConfigs = AZStd::vector<AZStd::shared_ptr<Physics::ShapeConfiguration>>;

    class ShapeConfigUpdateContext;

    class DynamicShapeColliderBase
    {
    public:
        void UpdateShapeConfigs(ShapeConfigs& shapeConfigs, PolygonPrismMeshUtils::Mesh2D& mesh, AZ::Entity* entity);

    protected:
        friend class ShapeConfigUpdateContext;

        bool m_simplePolygonErrorIssued = false;
        ShapeType m_shapeType = ShapeType::None;
        bool m_shapeTypeWarningIssued = false;
    };

    class ShapeConfigUpdateContext
    {
    private:
        friend class DynamicShapeColliderBase;

        ShapeConfigUpdateContext(ShapeConfigs& shapeConfigs, PolygonPrismMeshUtils::Mesh2D& mesh, AZ::Entity* entity, DynamicShapeColliderBase& state)
            : m_shapeConfigs(shapeConfigs)
            , m_mesh(mesh)
            , m_entity(entity)
            , m_state(state)
        {
            AZ_Assert(m_entity != nullptr, AZ_FUNCTION_SIGNATURE " - invalid parameters, expected non-null Entity");
        }

        void UpdateShapeConfigs();
        void UpdateBoxConfig();
        void UpdateCapsuleConfig();
        void UpdateSphereConfig();
        void UpdateCylinderConfig();
        void UpdatePolygonPrismDecomposition();
        void UpdatePolygonPrismDecomposition(const AZ::PolygonPrismPtr polygonPrismPtr);

        void AddMeshShape(const AZ::Vector3* pPoints, size_t nPoints);
        void AddMeshShape(const AZStd::vector<AZ::Vector3>& points);

        AZ::EntityId GetEntityId() const;
        AZ::Entity* GetEntity() const { return m_entity; }

        DynamicShapeColliderBase& m_state;
        ShapeConfigs& m_shapeConfigs;
        PolygonPrismMeshUtils::Mesh2D& m_mesh;
        AZ::Entity* m_entity = nullptr;
    };

    /// Component that provides a collider based on geometry from a shape component,
    /// built dynamically at entity activation time
    class DynamicShapeColliderComponent
        : public ShapeColliderComponent
    {
    public:
        AZ_COMPONENT(DynamicShapeColliderComponent, "{F9F4E3DB-5CC9-4C8D-9AB3-1901B5939CDB}", ShapeColliderComponent);
        static void Reflect(AZ::ReflectContext* context);

        DynamicShapeColliderComponent() = default;
        DynamicShapeColliderComponent(const Physics::ColliderConfiguration&);

        // BaseColliderComponent
        void UpdateScaleForShapeConfigs() override;

    private:
        Physics::ColliderConfiguration m_colliderConfig;
    };
}