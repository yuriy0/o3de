/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/RTTI/BehaviorContextUtilities.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Libraries/Comparison/Comparison.h>
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Libraries/Core/ExtractProperty.h>
#include <ScriptCanvas/Libraries/Core/ForEach.h>
#include <ScriptCanvas/Libraries/Core/FunctionCallNode.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/ReceiveScriptEvent.h>
#include <ScriptCanvas/Libraries/Core/Start.h>
#include <ScriptCanvas/Libraries/Logic/Break.h>
#include <ScriptCanvas/Libraries/Logic/Cycle.h>
#include <ScriptCanvas/Libraries/Logic/IsNull.h>
#include <ScriptCanvas/Libraries/Logic/Once.h>
#include <ScriptCanvas/Libraries/Logic/OrderedSequencer.h>
#include <ScriptCanvas/Libraries/Logic/WeightedRandomSequencer.h>
#include <ScriptCanvas/Libraries/Logic/While.h>
#include <ScriptCanvas/Libraries/Math/MathExpression.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorAdd.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorArithmetic.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorDiv.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorMul.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorSub.h>
#include <ScriptCanvas/Translation/Configuration.h>
#include <ScriptCanvas/Variable/VariableData.h>

#include "AbstractCodeModel.h"
#include "ParsingUtilities.h"
#include "ApcExtParsingUtilities.h"

namespace ApcExtParsingUtilitiesCpp
{

}

namespace ApcExtScriptCanvas
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Grammar;

    VariablePtr ConvertPureDataNodeToVariable(const Node&)
    {
        //if (auto pureDataNode = azrtti_cast<const PureData*>(&node))
        //{
        //    if (auto datum = pureDataNode->FindDatum(pureDataNode->GetSlotId(PureData::k_setThis)))
        //    {
        //        auto variable = AZStd::make_shared<Variable>();
        //        variable->m_datum = *datum;
        //        variable->m_isMember = true;
        //        variable->m_name = "convertedPureData";
        //        return variable;
        //    }
        //}

        return nullptr;
    }

    AZStd::optional<AZStd::pair<const ScriptCanvas::Nodes::Core::PropertySetterMetadata*, size_t>> GetClassPropertyWriteInfo(ScriptCanvas::Grammar::ExecutionTreeConstPtr execution)
    {
        const auto* setVariableNode = azrtti_cast<const ScriptCanvas::Nodes::Core::SetVariableNode*>(execution->GetId().m_node);
        if (!setVariableNode)
        {
            return AZStd::nullopt;
        }

        if (!execution->GetId().m_slot)
        {
            return AZStd::nullopt;
        }

        return setVariableNode->ApcExtGetPropertySetterMetaData(execution->GetId().m_slot->GetId());
    }

    bool IsClassPropertyWrite(ScriptCanvas::Grammar::ExecutionTreeConstPtr execution)
    {
        auto info = GetClassPropertyWriteInfo(execution);
        return info.has_value();
    }

    bool IsPureDataNode(const Node&)
    {
        return false;
        //return azrtti_istypeof<const ScriptCanvas::PureData>(&node);
    }    
}
