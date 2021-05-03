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

#include "ExecutionStateInterpreted.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>

#include "Execution/Interpreted/ExecutionStateInterpretedUtility.h"
#include "Execution/RuntimeComponent.h"

namespace ExecutionStateInterpretedCpp
{
    using namespace ScriptCanvas;

    AZ::Data::Asset<RuntimeAsset> GetSubgraphAssetForDebug(const AZ::Data::AssetId& id)
    {
        auto asset = AZ::Data::AssetManager::Instance().GetAsset<SubgraphInterfaceAsset>(id, AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();
        return asset;
    }
}

namespace ScriptCanvas
{
    ExecutionStateInterpreted::ExecutionStateInterpreted(const ExecutionStateConfig& config)
        : ExecutionState(config)
        , m_interpretedAsset(config.runtimeData.m_script)
        , m_runtimeData(config.runtimeData)
    {}
    
    void ExecutionStateInterpreted::ClearLuaRegistryIndex()
    {
        m_luaRegistryIndex = LUA_NOREF;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolIn(size_t index) const
    {
        return index < m_component->GetAssetData().m_debugMap.m_ins.size()
            ? &(m_component->GetAssetData().m_debugMap.m_ins[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolIn(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateInterpretedCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->GetData().m_debugMap.m_ins.size()
            ? &(asset.Get()->GetData().m_debugMap.m_ins[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolOut(size_t index) const
    {
        return index < m_component->GetAssetData().m_debugMap.m_outs.size()
            ? &(m_component->GetAssetData().m_debugMap.m_outs[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolOut(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateInterpretedCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->GetData().m_debugMap.m_outs.size()
            ? &(asset.Get()->GetData().m_debugMap.m_outs[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolReturn(size_t index) const
    {
        return index < m_component->GetAssetData().m_debugMap.m_returns.size()
            ? &(m_component->GetAssetData().m_debugMap.m_returns[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolReturn(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateInterpretedCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->GetData().m_debugMap.m_returns.size()
            ? &(asset.Get()->GetData().m_debugMap.m_returns[index])
            : nullptr;
    }

    const Grammar::DebugDataSource* ExecutionStateInterpreted::GetDebugSymbolVariableChange(size_t index) const
    {
        return index < m_component->GetAssetData().m_debugMap.m_variables.size()
            ? &(m_component->GetAssetData().m_debugMap.m_variables[index])
            : nullptr;
    }

    const Grammar::DebugDataSource* ExecutionStateInterpreted::GetDebugSymbolVariableChange(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateInterpretedCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->GetData().m_debugMap.m_variables.size()
            ? &(asset.Get()->GetData().m_debugMap.m_variables[index])
            : nullptr;
    }

    ExecutionMode ExecutionStateInterpreted::GetExecutionMode() const
    {
        return ExecutionMode::Interpreted;
    }

    int ExecutionStateInterpreted::GetLuaRegistryIndex() const
    {
        return m_luaRegistryIndex;
    }

    lua_State* ExecutionStateInterpreted::LoadLuaScript()
    {
        AZ::ScriptLoadResult result{};
        AZ::ScriptSystemRequestBus::BroadcastResult(result, &AZ::ScriptSystemRequests::LoadAndGetNativeContext, m_interpretedAsset, AZ::k_scriptLoadBinary, AZ::ScriptContextIds::DefaultScriptContextId);
        AZ_Assert(result.status != AZ::ScriptLoadResult::Status::Failed, "ExecutionStateInterpreted script asset was valid but failed to load.");
        AZ_Assert(result.lua, "Must have a default script context and a lua_State");
        AZ_Assert(lua_istable(result.lua, -1), "No run-time execution was available for this script");
        m_luaState = result.lua;
        return result.lua;
    }

    void ExecutionStateInterpreted::ReferenceExecutionState()
    {
        AZ_Assert(m_luaRegistryIndex == LUA_NOREF, "ExecutionStateInterpreted already in the Lua registry and risks double deletion");
        // Lua: instance
        m_luaRegistryIndex = luaL_ref(m_luaState, LUA_REGISTRYINDEX);

        // Once lua object state is ready, connect to runtime buses
        if (m_component && m_component->GetEntity())
        {
            const AZ::EntityId attachedEntity = m_component->GetEntity()->GetId();
            VariableRuntimeRequestBus::Handler::BusConnect(attachedEntity);
        }
        else
        {
            AZ_Warning("ScriptCanvas", false, AZ_FUNCTION_SIGNATURE " - attached component is invalid, runtime buses will not be serviced!");
        }
    }

    void ExecutionStateInterpreted::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<ExecutionStateInterpreted>()
                ;
        }
    }

    void ExecutionStateInterpreted::ReleaseExecutionState()
    {
        if (m_luaRegistryIndex != LUA_NOREF)
        {
            ReleaseExecutionStateUnchecked();
        }
    }

    void ExecutionStateInterpreted::ReleaseExecutionStateUnchecked()
    {
        luaL_unref(m_luaState, LUA_REGISTRYINDEX, m_luaRegistryIndex);
        m_luaRegistryIndex = LUA_NOREF;
        VariableRuntimeRequestBus::Handler::BusDisconnect();
    }

    VariableSetResult ExecutionStateInterpreted::SetVariable(AZStd::string_view name, const AZStd::any& value)
    {
        const auto registryIndex = GetLuaRegistryIndex();
        if (registryIndex == LUA_NOREF)
        {
            AZ_Assert(false, "ExecutionStateInterpreted::SetVariable called but Initialize is was never called");
            return VariableAccessError::StateError;
        }

        // Check variable name
        const auto& varInfoMap = m_runtimeData.m_input.m_runtimeVariableInfo;
        auto varInfoIt = varInfoMap.find(name);
        if (varInfoIt == varInfoMap.end())
        {
            return VariableAccessError::NonExistentVariable;
        }

        // Check variable type
        if (varInfoIt->second.m_type != value.type())
        {
            return VariableAccessError::WrongTypeValue;
        }

        lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, registryIndex);
        // Lua: instance

        AZStd::string_view internalName = varInfoIt->second.m_name;
        lua_pushlstring(m_luaState, internalName.data(), internalName.size());
        // Lua: instance, key

        AZ::ScriptValue<AZStd::any>::StackPush(m_luaState, value);
        // Lua: instance, key, value

        lua_settable(m_luaState, -3); // TODO: or lua_rawset?
        // Lua: instance

        lua_pop(m_luaState, 1);
        // Lua: 

        return VariableSetResult();
    }

    VariableGetResult ExecutionStateInterpreted::GetVariable(AZStd::string_view name)
    {
        const auto registryIndex = GetLuaRegistryIndex();
        if (registryIndex == LUA_NOREF)
        {
            AZ_Assert(false, "ExecutionStateInterpreted::GetVariable called but Initialize is was never called");
            return VariableAccessError::StateError;
        }

        // Check variable name
        const auto& varInfoMap = m_runtimeData.m_input.m_runtimeVariableInfo;
        auto varInfoIt = varInfoMap.find(name);
        if (varInfoIt == varInfoMap.end())
        {
            return VariableAccessError::NonExistentVariable;
        }

        lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, registryIndex);
        // Lua: instance

        AZStd::string_view internalName = varInfoIt->second.m_name;
        lua_pushlstring(m_luaState, internalName.data(), internalName.size());
        // Lua: instance, key

        lua_gettable(m_luaState, -2);
        // Lua: instance, value

        auto res = AZ::ScriptValue<AZStd::any>::StackRead(m_luaState, -1);
        // Lua: instance, value

        lua_pop(m_luaState, 2);
        // Lua:

        return VariableGetResult(AZStd::move(res));
    }

} 
