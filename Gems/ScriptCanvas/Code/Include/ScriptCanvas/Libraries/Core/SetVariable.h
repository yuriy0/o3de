/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/EditContextConstants.inl>

#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/SlotNames.h>
#include <ScriptCanvas/Data/PropertyTraits.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableCore.h>

#include <Include/ScriptCanvas/Libraries/Core/SetVariable.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            struct PropertySetterMetadata
            {
                AZ_TYPE_INFO(PropertySetterMetadata, "{7C5DBCB2-6887-4810-98D6-4E4D9D33BC1A}");
                AZ_CLASS_ALLOCATOR(PropertySetterMetadata, AZ::SystemAllocator, 0);
                static void Reflect(AZ::ReflectContext* reflectContext);

                SlotId m_propertySlotId;
                SlotId m_signalSlotId;
                Data::Type m_propertyType;
                AZStd::string m_propertyName;
                Data::SetterFunction m_setterFunction;
            };

			//! Provides a node to set the value of a variable
            class SetVariableNode
                : public Node
                , protected VariableNotificationBus::Handler
                , protected VariableNodeRequestBus::Handler
            {
            public:

                SCRIPTCANVAS_NODE(SetVariableNode);

                // Node...
                void CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;
                bool ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;
                bool RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) override;
                ////

                //// VariableNodeRequestBus...
                void SetId(const VariableId& variableId) override;
                const VariableId& GetId() const override;
                ////

                const SlotId& GetDataInSlotId() const;
                const SlotId& GetDataOutSlotId() const;


                //////////////////////////////////////////////////////////////////////////
                // Translation
                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                VariableId GetVariableIdRead(const Slot*) const override;

                VariableId GetVariableIdWritten(const Slot*) const override;

                const Slot* GetVariableOutputSlot() const override;

                PropertyFields GetPropertyFields() const override;
                // Translation
                //////////////////////////////////////////////////////////////////////////


            protected:

                void OnInit() override;
                void OnPostActivate() override;

                void AddSlots();
                void RemoveSlots();
                void AddPropertySlots(const Data::Type& type);
                void ClearPropertySlots();
                void RefreshPropertyFunctions();

                GraphScopedVariableId GetScopedVariableId() const;

                GraphVariable* ModVariable() const;

                void OnIdChanged(const VariableId& oldVariableId);
                AZStd::vector<AZStd::pair<VariableId, AZStd::string>> GetGraphVariables() const;

                // VariableNotificationBus
                void OnVariableRemoved() override;
                ////

                VariableId m_variableId;

                SlotId m_variableDataInSlotId;
                SlotId m_variableDataOutSlotId;

                AZStd::vector<Data::PropertyMetadata> m_propertyAccounts;
                AZStd::vector<PropertySetterMetadata> m_propertySetters;

                AZStd::string_view  m_variableName;
                ModifiableDatumView m_variableView;

            public:
                AZStd::optional<AZStd::pair<const PropertySetterMetadata*, size_t>> ApcExtGetPropertySetterMetaData(SlotId slotID) const;
            };
        }
    }
}
