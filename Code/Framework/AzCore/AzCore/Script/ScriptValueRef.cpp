#include <AzCore/Script/ScriptValueRef.h>

#include <AzCore/Script/lua/lua.h>

namespace AZ {
    void ScriptValueRef::Unref(lua_State * lua, int * p) {
        luaL_unref(lua, LUA_REGISTRYINDEX, *p);
        AZStd::checked_delete(p);
    }

    int* ScriptValueRef::Init(lua_State * lua, int stackIndex) {
        lua_pushvalue(lua, stackIndex);
        int* p_ref = new int;
        *p_ref = luaL_ref(lua, LUA_REGISTRYINDEX);
        return p_ref;
    }

    ScriptValueRef::ScriptValueRef(lua_State * lua, int stackIndex)
        : m_lua(lua)
        , RefT(Init(lua, stackIndex), AZStd::function<void(int*)>([lua](int* p) { Unref(lua, p); }))
    {}

    void ScriptValueRef::StackPush() const {
        lua_rawgeti(m_lua, LUA_REGISTRYINDEX, *get());
    }

    int ScriptValueRef::GetRef() const {
        return *get();
    }

    lua_State* ScriptValueRef::GetLuaState() const {
        return m_lua;
    }
}