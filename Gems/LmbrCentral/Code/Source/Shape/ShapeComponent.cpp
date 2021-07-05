/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LmbrCentral_precompiled.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Math/Random.h>

namespace LmbrCentral
{
    struct RandomDistributionType_Holder {
        AZ_TYPE_INFO(RandomDistributionType_Holder, "{EB7145DF-4DE4-4DED-A440-7401B379A686}");
    };

    // BehaviorConext ShapeComponentNotificationsBus forwarder
    class BehaviorShapeComponentNotificationsBusHandler : public ShapeComponentNotificationsBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorShapeComponentNotificationsBusHandler, "{A82C9481-693B-4010-9812-1A21B106FCC0}", AZ::SystemAllocator,
            OnShapeChanged);

        void OnShapeChanged(ShapeChangeReasons changeReason) override
        {
            Call(FN_OnShapeChanged, changeReason);
        }
    };
    
    void ShapeComponentGeneric::Reflect(AZ::ReflectContext* context)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<ShapeComponentRequestsBus>("ShapeComponentRequestsBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "shape")
                ->Event("GetShapeType", &ShapeComponentRequestsBus::Events::GetShapeType)
                ->Event("IsPointInside", &ShapeComponentRequestsBus::Events::IsPointInside)
                ->Event("DistanceFromPoint", &ShapeComponentRequestsBus::Events::DistanceFromPoint)
                ->Event("DistanceSquaredFromPoint", &ShapeComponentRequestsBus::Events::DistanceSquaredFromPoint)
                ->Event("GetEncompassingAabb", &ShapeComponentRequestsBus::Events::GetEncompassingAabb)
                ->Event("IntersectRay", &ShapeComponentRequestsBus::Events::IntersectRay)
                ->Event("GenerateRandomPointInside", &ShapeComponentRequestsBus::Events::GenerateRandomPointInside)
                ;

            behaviorContext->Class<RandomDistributionType_Holder>("RandomDistributionType")
                ->Constant("Normal", BehaviorConstant(AZ::RandomDistributionType::Normal))
                ->Constant("UniformReal", BehaviorConstant(AZ::RandomDistributionType::UniformReal))
                ;

            behaviorContext->Enum<(int)ShapeComponentNotifications::ShapeChangeReasons::TransformChanged>("ShapeChangeReasons_TransformChanged")
                           ->Enum<(int)LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged>("ShapeChangeReasons_ShapeChanged");
            
            behaviorContext->EBus<ShapeComponentNotificationsBus>("ShapeComponentNotificationsBus")
                ->Handler<BehaviorShapeComponentNotificationsBusHandler>()
                ;
        }
    }

    void ShapeComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShapeComponentConfig>()
                ->Version(1)
                ->Field("DrawColor", &ShapeComponentConfig::m_drawColor)
                ->Field("IsFilled", &ShapeComponentConfig::m_filled)
            ;
        }
    }
} // namespace LmbrCentral
