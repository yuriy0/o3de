#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/functional.h>

struct lua_State;

namespace AZ {
    // Lua values stored on the stack are not generally compatible with the C++ paradigm of resource acquisition and relinquishing.
    // Sometimes we would like to own Lua values directly in the C++ runtime.
    // Lua permits this through registry values, in which values stored in the registry table are not garbage collected.
    // This class encapsulated registry values, but this is an implementation detail: the main semantics are as follows:
    //   - You create a RefScriptValue from a Lua state and a stack index; the value at that index is stored in the new ScriptValue
    //   - You can copy/move the ScriptValue, which keeps the Lua value alive registry while there is still a reference (via shared_ptr)
    //   - You can get a copy of the stored value by calling `StackPush`, which pushes the value to the top of the stack
    class ScriptValueRef
        : private AZStd::shared_ptr<int>
    {
        using RefT = AZStd::shared_ptr<int>;
        lua_State* m_lua;

        static void Unref(lua_State* lua, int* p);
        static int* Init(lua_State* lua, int stackIndex);

    public:
        AZ_TYPE_INFO(ScriptValueRef, "{97525E4B-9801-40A6-B3D5-FC39163CFF5C}");
        AZ_CLASS_ALLOCATOR(ScriptValueRef, AZ::SystemAllocator, 0);

        ScriptValueRef(lua_State* lua, int stackIndex = -1);

        void StackPush() const;
        int GetRef() const;
        lua_State* GetLuaState() const;
    };
}
