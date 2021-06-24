/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SetVariable.h"

#include <Core/ExecutionNotificationsBus.h>
#include <Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationIds.h>
#include <ScriptCanvas/Grammar/ParsingUtilities.h>
#include <ScriptCanvas/Translation/GraphToLuaUtility.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            AZ::Outcome<DependencyReport, void> SetVariableNode::GetDependencies() const
            {
                if (auto datum = ModVariable()->GetDatum())
                {
                    return AZ::Success(DependencyReport::NativeLibrary(Data::GetName(datum->GetType()).c_str()));
                }
                else
                {
                    return AZ::Failure();
                }
            }

            PropertyFields SetVariableNode::GetPropertyFields() const
            {
                PropertyFields propertyFields;
                for (auto&& propertyAccount : m_propertyAccounts)
                {
                    propertyFields.emplace_back(propertyAccount.m_propertyName, propertyAccount.m_propertySlotId);
                }

                return propertyFields;
            }

            void SetVariableNode::OnInit()
            {
                VariableNodeRequestBus::Handler::BusConnect(GetEntityId());
            }

            void SetVariableNode::OnPostActivate()
            {
                if (m_variableId.IsValid())
                {
                    RefreshPropertyFunctions();
                    PopulateNodeType();
                    VariableNotificationBus::Handler::BusConnect(GetScopedVariableId());
                }
            }

            void SetVariableNode::OnInputSignal(const SlotId& slotID)
            {
                if (slotID == GetSlotId(GetInputSlotName()))
                {
                    const Datum* sourceDatum = FindDatum(m_variableDataInSlotId);                    

                    if (sourceDatum && m_variableView.IsValid())
                    {
                        m_variableView.AssignToDatum((*sourceDatum));

                        SC_EXECUTION_TRACE_VARIABLE_CHANGE((m_variableId), (CreateVariableChange((*m_variableView.GetDatum()), m_variableId)));
                    }
                    
                    Slot* resultSlot = GetSlot(m_variableDataOutSlotId);
                    if (resultSlot && m_variableView.IsValid())
                    {
                        const Datum* variableDatum = m_variableView.GetDatum();

                        PushOutput((*variableDatum), *resultSlot);

                        // Push the data for each property slot out as well
                        for (auto&& propertyAccount : m_propertyAccounts)
                        {
                            Slot* propertySlot = GetSlot(propertyAccount.m_propertySlotId);
                            if (propertySlot && propertyAccount.m_getterFunction)
                            {
                                auto outputOutcome = propertyAccount.m_getterFunction((*variableDatum));

                                if (!outputOutcome)
                                {
                                    SCRIPTCANVAS_REPORT_ERROR((*this), outputOutcome.TakeError().data());
                                    return;
                                }
                                PushOutput(outputOutcome.TakeValue(), *propertySlot);
                            }
                        }
                    }

                    SignalOutput(GetSlotId(GetOutputSlotName()));
                }
                else
                {
                    auto setterIt = AZStd::find_if(
                        m_propertySetters.begin(),
                        m_propertySetters.end(),
                        [&](const PropertySetterMetadata& setterData)
                        {
                            return setterData.m_signalSlotId == slotID;    
                        }
                    );
                    if (setterIt != m_propertySetters.end())
                    {
                        const PropertySetterMetadata& setterData = *setterIt;
                        
                        SC_EXECUTION_TRACE_ANNOTATE_NODE((*this), CreateAnnotationData());

                        // Get the source data
                        const Datum* sourceDatum = FindDatum(m_variableDataInSlotId);
                        const Datum* subFieldDatum = FindDatum(setterData.m_propertySlotId);

                        // Assign the data
                        if (sourceDatum && m_variableView.IsValid() && setterData.m_setterFunction)
                        {
                            Datum sourceDatumCopy = *sourceDatum;
                            auto res = setterData.m_setterFunction(sourceDatumCopy, *subFieldDatum);

                            if (!res.IsSuccess())
                            {
                                AZ_Error("Script Canvas", false,
                                    "Failed to call property (%s : %s) setter method: %s",
                                    setterData.m_propertyName.c_str(), Data::GetName(setterData.m_propertyType).data(),
                                    res.GetError().c_str()
                                );
                            }

                            m_variableView.AssignToDatum(AZStd::move(sourceDatumCopy));

                            const Datum* variableDatum = m_variableView.GetDatum();

                            SC_EXECUTION_TRACE_VARIABLE_CHANGE((m_variableId), (CreateVariableChange((*variableDatum), m_variableId)));
                        }
                    }

                    Slot* resultSlot = GetSlot(m_variableDataOutSlotId);
                    if (resultSlot && m_variableView.IsValid())
                    {
                        const Datum* variableDatum = m_variableView.GetDatum();

                        PushOutput((*variableDatum), *resultSlot);

                        // Push the data for each property slot out as well
                        for (auto&& propertyAccount : m_propertyAccounts)
                        {
                            Slot* propertySlot = GetSlot(propertyAccount.m_propertySlotId);
                            if (propertySlot && propertyAccount.m_getterFunction)
                            {
                                auto outputOutcome = propertyAccount.m_getterFunction((*variableDatum));

                                if (!outputOutcome)
                                {
                                    SCRIPTCANVAS_REPORT_ERROR((*this), outputOutcome.TakeError().data());
                                    return;
                                }
                                PushOutput(outputOutcome.TakeValue(), *propertySlot);
                            }
                        }
                    }

                    SignalOutput(GetSlotId(GetOutputSlotName()));
                }
            }
            VariableId SetVariableNode::GetVariableIdRead(const Slot*) const
            {
                return m_variableId;
            }

            VariableId SetVariableNode::GetVariableIdWritten(const Slot*) const
            {
                return m_variableId;
            }

            const Slot* SetVariableNode::GetVariableOutputSlot() const
            {
                return GetSlot(m_variableDataOutSlotId);
            }

            void SetVariableNode::CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
            {
                if (m_variableId.IsValid())
                {
                    variableIds.insert(m_variableId);
                }

                return Node::CollectVariableReferences(variableIds);
            }

            bool SetVariableNode::ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
            {
                if (m_variableId.IsValid() && variableIds.count(m_variableId) > 0)
                {
                    return true;
                }

                return Node::ContainsReferencesToVariables(variableIds);
            }

            bool SetVariableNode::RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds)
            {
                // These nodes should just be deleted when the variable they reference is removed. Don't try to 
                // update the variable they reference.
                if (m_variableId.IsValid() && variableIds.count(m_variableId) > 0)
                {
                    return false;
                }

                return Node::RemoveVariableReferences(variableIds);
            }

            void SetVariableNode::SetId(const VariableId& variableDatumId)
            {
                if (m_variableId != variableDatumId)
                {
                    VariableId oldVariableId = m_variableId;
                    m_variableId = variableDatumId;

                    VariableNotificationBus::Handler::BusDisconnect();

                    ScriptCanvas::Data::Type oldType = ScriptCanvas::Data::Type::Invalid();

                    if (m_variableDataInSlotId.IsValid())
                    {
                        oldType = GetSlotDataType(m_variableDataInSlotId);
                    }

                    ScriptCanvas::Data::Type newType = ScriptCanvas::Data::Type::Invalid();
                    VariableRequestBus::EventResult(newType, GetScopedVariableId(), &VariableRequests::GetType);

                    if (oldType != newType)
                    {
                        ScopedBatchOperation scopedBatchOperation(AZ_CRC("SetVariableIdChanged", 0xc072e633));
                        RemoveSlots();
                        AddSlots();
                    }

                    if (m_variableId.IsValid())
                    {
                        VariableNotificationBus::Handler::BusConnect(GetScopedVariableId());
                    }

                    VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableIdChanged, oldVariableId, m_variableId);

                    PopulateNodeType();
                }
            }

            const VariableId& SetVariableNode::GetId() const
            {
                return m_variableId;
            }

            const SlotId& SetVariableNode::GetDataInSlotId() const
            {
                return m_variableDataInSlotId;
            }

            const SlotId& SetVariableNode::GetDataOutSlotId() const
            {
                return m_variableDataOutSlotId;
            }

            GraphVariable* SetVariableNode::ModVariable() const
            {
                GraphVariable* graphVariable = FindGraphVariable(m_variableId);

                AZ_Warning("ScriptCanvas", graphVariable != nullptr, "Unknown variable referenced by Id - %s", m_variableId.ToString().data());
                AZ_Warning("ScriptCanvas", (graphVariable == nullptr || graphVariable->GetVariableId() == m_variableId), "Mismatch in SetVariableNode: VariableId %s requested but found VariableId %s", m_variableId.ToString().data(), graphVariable->GetVariableId().ToString().data());

                return graphVariable;
            }

            void SetVariableNode::AddSlots()
            {
                if (m_variableId.IsValid())
                {
                    GraphScopedVariableId scopedVariableId = GetScopedVariableId();
                    AZStd::string_view varName;
                    Data::Type varType;
                    VariableRequestBus::EventResult(varName, scopedVariableId, &VariableRequests::GetName);
                    VariableRequestBus::EventResult(varType, scopedVariableId, &VariableRequests::GetType);

                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = Data::GetName(varType);
                        slotConfiguration.SetConnectionType(ConnectionType::Input);
                        slotConfiguration.ConfigureDatum(AZStd::move(Datum(varType, Datum::eOriginality::Copy)));

                        m_variableDataInSlotId = AddSlot(slotConfiguration);
                    }

                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = Data::GetName(varType);
                        slotConfiguration.SetConnectionType(ConnectionType::Output);
                        slotConfiguration.SetType(varType);

                        m_variableDataOutSlotId = AddSlot(slotConfiguration);
                    }

                    AddPropertySlots(varType);
                }
            }

            void SetVariableNode::RemoveSlots()
            {
                ClearPropertySlots();

                SlotId oldVariableDataInSlotId;
                AZStd::swap(oldVariableDataInSlotId, m_variableDataInSlotId);

                if (oldVariableDataInSlotId.IsValid())
                {
                    RemoveSlot(oldVariableDataInSlotId);
                }

                SlotId oldVariableDataOutSlotId;
                AZStd::swap(oldVariableDataOutSlotId, m_variableDataOutSlotId);

                if (oldVariableDataOutSlotId.IsValid())
                {
                    RemoveSlot(oldVariableDataOutSlotId);
                }
            }

            void SetVariableNode::OnIdChanged(const VariableId& oldVariableId)
            {
                if (m_variableId != oldVariableId)
                {
                    VariableId newVariableId = m_variableId;
                    m_variableId = oldVariableId;
                    SetId(newVariableId);
                }
            }

            AZStd::vector<AZStd::pair<VariableId, AZStd::string>> SetVariableNode::GetGraphVariables() const
            {
                AZStd::vector<AZStd::pair<VariableId, AZStd::string>> varNameToIdList;

                if (m_variableId.IsValid())
                {
                    ScriptCanvas::Data::Type baseType = ScriptCanvas::Data::Type::Invalid();
                    VariableRequestBus::EventResult(baseType, GetScopedVariableId(), &VariableRequests::GetType);

                    const AZStd::unordered_map<VariableId, GraphVariable>* variableMap = GetRuntimeBus()->GetVariables();

                    if (variableMap && baseType.IsValid())
                    {
                        for (const auto& variablePair : *variableMap)
                        {
                            ScriptCanvas::Data::Type variableType = variablePair.second.GetDatum()->GetType();

                            if (variableType == baseType)
                            {
                                varNameToIdList.emplace_back(variablePair.first, variablePair.second.GetVariableName());
                            }
                        }

                        AZStd::sort(varNameToIdList.begin(), varNameToIdList.end(), [](const AZStd::pair<VariableId, AZStd::string>& lhs, const AZStd::pair<VariableId, AZStd::string>& rhs)
                        {
                            return lhs.second < rhs.second;
                        });
                    }
                }

                return varNameToIdList;
            }

            void SetVariableNode::OnVariableRemoved()
            {
                VariableNotificationBus::Handler::BusDisconnect();
                VariableId removedVariableId;
                AZStd::swap(removedVariableId, m_variableId);
                {
                    ScopedBatchOperation scopedBatchOperation(AZ_CRC("SetVariableRemoved", 0xd7da59f5));
                    RemoveSlots();
                }
                VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableRemovedFromNode, removedVariableId);
            }

            AnnotateNodeSignal SetVariableNode::CreateAnnotationData()
            {
                AZ::EntityId assetNodeId = GetRuntimeBus()->FindAssetNodeIdByRuntimeNodeId(GetEntityId());
                return AnnotateNodeSignal(CreateGraphInfo(GetOwningScriptCanvasId(), GetGraphIdentifier()), AnnotateNodeSignal::AnnotationLevel::Info, m_variableName, AZ::NamedEntityId(assetNodeId, GetNodeName()));
            }

            void SetVariableNode::AddPropertySlots(const Data::Type& type)
            {
                Data::GetterContainer getterFunctions = Data::ExplodeToGetters(type);
                for (const auto& getterWrapperPair : getterFunctions)
                {
                    const AZStd::string& propertyName = getterWrapperPair.first;
                    const Data::GetterWrapper& getterWrapper = getterWrapperPair.second;
                    Data::PropertyMetadata propertyAccount;
                    propertyAccount.m_propertyType = getterWrapper.m_propertyType;
                    propertyAccount.m_propertyName = propertyName;

                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = AZStd::string::format("%s: %s", propertyName.data(), Data::GetName(getterWrapper.m_propertyType).data());
                        slotConfiguration.SetType(getterWrapper.m_propertyType);
                        slotConfiguration.SetConnectionType(ConnectionType::Output);

                        propertyAccount.m_propertySlotId = AddSlot(slotConfiguration);

                    }

                    propertyAccount.m_getterFunction = getterWrapper.m_getterFunction;
                    m_propertyAccounts.push_back(propertyAccount);
                }

                Data::SetterContainer setterFunctions = Data::ExplodeToSetters(type);
                for (const auto& setterWrapperPair : setterFunctions)
                {
                    const AZStd::string& propertyName = setterWrapperPair.first;
                    const Data::SetterWrapper& getterWrapper = setterWrapperPair.second;
                    PropertySetterMetadata propertyAccount;
                    propertyAccount.m_propertyType = getterWrapper.m_propertyType;
                    propertyAccount.m_propertyName = propertyName;

                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = AZStd::string::format("%s: %s", propertyName.data(), Data::GetName(getterWrapper.m_propertyType).data());
                        slotConfiguration.SetType(getterWrapper.m_propertyType);
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        propertyAccount.m_propertySlotId = AddSlot(slotConfiguration);

                    }

                    {
                        ExecutionSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = AZStd::string::format("Update %s: %s", propertyName.data(), Data::GetName(getterWrapper.m_propertyType).data());
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        propertyAccount.m_signalSlotId = AddSlot(slotConfiguration);
                    }

                    propertyAccount.m_setterFunction = getterWrapper.m_setterFunction;
                    m_propertySetters.push_back(propertyAccount);
                }
            }

            void SetVariableNode::ClearPropertySlots()
            {
                {
                    auto oldPropertyAccounts = AZStd::move(m_propertyAccounts);
                    m_propertyAccounts.clear();
                    for (auto&& propertyAccount : oldPropertyAccounts)
                    {
                        RemoveSlot(propertyAccount.m_propertySlotId);
                    }
                }

                {
                    auto oldPropertyAccounts = AZStd::move(m_propertySetters);
                    m_propertySetters.clear();
                    for (auto&& propertyAccount : oldPropertyAccounts)
                    {
                        RemoveSlot(propertyAccount.m_propertySlotId);
                        RemoveSlot(propertyAccount.m_signalSlotId);
                    }
                }
            }

            void SetVariableNode::RefreshPropertyFunctions()
            {
                GraphVariable* variable = FindGraphVariable(m_variableId);

                if (variable == nullptr)
                {
                    return;
                }

                Data::Type sourceType = variable->GetDataType();

                if (!sourceType.IsValid())
                {
                    return;
                }

                auto getterWrapperMap = Data::ExplodeToGetters(sourceType);

                for (auto& propertyAccount : m_propertyAccounts)
                {
                    if (!propertyAccount.m_getterFunction)
                    {
                        auto foundPropIt = getterWrapperMap.find(propertyAccount.m_propertyName);
                        if (foundPropIt != getterWrapperMap.end() && propertyAccount.m_propertyType.IS_A(foundPropIt->second.m_propertyType))
                        {
                            propertyAccount.m_getterFunction = foundPropIt->second.m_getterFunction;
                        }
                        else
                        {
                            AZ_Error("Script Canvas", false, "Property (%s : %s) getter method could not be found in Data::PropertyTraits or the property type has changed."
                                " Output will not be pushed on the property's slot.",
                                propertyAccount.m_propertyName.c_str(), Data::GetName(propertyAccount.m_propertyType).data());
                        }
                    }
                }

                auto setterWrapperMap = Data::ExplodeToSetters(sourceType);

                for (auto& propertyAccount : m_propertySetters)
                {
                    if (!propertyAccount.m_setterFunction)
                    {
                        auto foundPropIt = setterWrapperMap.find(propertyAccount.m_propertyName);
                        if (foundPropIt != setterWrapperMap.end() && propertyAccount.m_propertyType.IS_A(foundPropIt->second.m_propertyType))
                        {
                            propertyAccount.m_setterFunction = foundPropIt->second.m_setterFunction;
                        }
                        else
                        {
                            AZ_Error("Script Canvas", false, "Property (%s : %s) setter method could not be found in Data::PropertyTraits or the property type has changed.",
                                propertyAccount.m_propertyName.c_str(), Data::GetName(propertyAccount.m_propertyType).data());
                        }
                    }
                }
            }

            GraphScopedVariableId SetVariableNode::GetScopedVariableId() const
            {
                return GraphScopedVariableId(GetOwningScriptCanvasId(), m_variableId);
            }

            AZStd::optional<AZStd::pair<const PropertySetterMetadata*, size_t>> SetVariableNode::ApcExtGetPropertySetterMetaData(SlotId slotId) const
            {
                for (size_t index = 0, sentinel = m_propertySetters.size(); index != sentinel; ++index)
                {
                    auto& candidate = m_propertySetters[index];

                    if (candidate.m_signalSlotId == slotId)
                    {
                        return AZStd::make_pair(&candidate, index + 1);
                    }
                }

                return AZStd::nullopt;
            }

            void PropertySetterMetadata::Reflect(AZ::ReflectContext* context)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<PropertySetterMetadata>()
                        ->Field("m_propertySlotId", &PropertySetterMetadata::m_propertySlotId)
                        ->Field("m_signalSlotId", &PropertySetterMetadata::m_signalSlotId)
                        ->Field("m_propertyType", &PropertySetterMetadata::m_propertyType)
                        ->Field("m_propertyName", &PropertySetterMetadata::m_propertyName)
                        ->Version(2)
                        ;
                }
            }
        }
    }
}
