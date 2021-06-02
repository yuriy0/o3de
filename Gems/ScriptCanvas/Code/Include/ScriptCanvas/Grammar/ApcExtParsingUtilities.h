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

#pragma once

#include <ScriptCanvas/Libraries/Core/SetVariable.h>

#include "Primitives.h"
#include "PrimitivesExecution.h"

namespace ApcExtScriptCanvas
{
    ScriptCanvas::Grammar::VariablePtr ConvertPureDataNodeToVariable(const ScriptCanvas::Node& node);

    AZStd::optional<AZStd::pair<const ScriptCanvas::Nodes::Core::PropertySetterMetadata*, size_t>> GetClassPropertyWriteInfo(ScriptCanvas::Grammar::ExecutionTreeConstPtr execution);

    bool IsClassPropertyWrite(ScriptCanvas::Grammar::ExecutionTreeConstPtr execution);

    bool IsPureDataNode(const ScriptCanvas::Node& node);
}
