#pragma once

#include <AzCore/std/optional.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Script/lua/lua.h>

namespace AZ {
	/* APC TODO: Merge with upstream implementation
	
    template<class T>
    struct OnDemandReflection<AZStd::optional<T>> {
        using ThisType = AZStd::optional<T>;

        static void ToLua(lua_State* lua, AZ::BehaviorValueParameter& param)
        {
            ThisType& value = *param.GetAsUnsafe<ThisType>();
            if (value)
            {
                AZ::ScriptValue<T>::StackPush(lua, *value);
            }
            else
            {
                lua_pushnil(lua);
            }
        }

        static bool FromLuaCheck(lua_State* lua, int stackIndex, AZ::BehaviorValueParameter& outVal, void* storage)
        {
            static constexpr bool T_is_class = AZStd::is_class<T>::value;
            if (T_is_class && AZ::Internal::LuaIsClass(lua, stackIndex, &AZ::AzTypeInfo<T>::Uuid()))
            {
                // If the enclosed type is a class, check if we can read it and the do read it
                // NB: if we don't check using 'LuaIsClass' then 'StackRead' derefences a null pointer!
                outVal.Set(new (storage) ThisType( AZ::ScriptValue<T>::StackRead(lua, stackIndex) ));
                return true;
            }
            else if (!T_is_class)
            {
                // If T is not a class, check if the value at the index is nil
                // If so, set the output value to nil, other try to read whatever value is
                // there as the target type (which may or may not suceed). Hopefully, for non-class
                // types, ScriptVlaue::StackRead should not do anything nasty, instead just return a
                // default dummy value (bad failure mode) or produce a Lua error (better failure mode).
                // The best failure mode would be if ScriptValue<T>::StackRead returned some value indicating
                // if it did actually read a value of the given type, and if so what that value is (i.e. an `optional<T>').
                if (Internal::azlua_isnil(lua, stackIndex))
                {
                    outVal.Set(new (storage) ThisType{});
                }
                else
                {
                    outVal.Set(new (storage) ThisType(AZ::ScriptValue<T>::StackRead(lua, stackIndex)));
                }
                return true;
            }
            else {
                return false;
            }
        }

        static bool FromLua(lua_State* lua, int stackIndex, AZ::BehaviorValueParameter& value, AZ::BehaviorClass*, AZ::ScriptContext::StackVariableAllocator* stackTempAllocator)
        {
            bool didAllocation = false;
            void* thisPtr = nullptr;
            if (value.m_value) {
                thisPtr = value.m_value;
            } else if (stackTempAllocator != nullptr) {
                thisPtr = stackTempAllocator->allocate(sizeof(ThisType), AZStd::alignment_of<ThisType>::value);
                if (thisPtr == nullptr) {
                    AZ_Assert(false, "optional::FromLua failed to allocate temp memory!");
                    return false;
                }
                didAllocation = true;
            } else {
                AZ_Assert(false, "optional::FromLua called without temp storage and null allocator!");
                return false;
            }

            bool ret = FromLuaCheck(lua, stackIndex, value, thisPtr);

            if (!ret) {
                if (didAllocation) stackTempAllocator->deallocate(thisPtr, sizeof(ThisType), AZStd::alignment_of<ThisType>::value);
                return false;
            }

            return ret;
        }

        static void Reflect(AZ::ReflectContext* context) {
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context)) {
                behaviorContext->Class<ThisType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Ignore, true)

                    ->Attribute(AZ::Script::Attributes::ReaderWriterOverride, AZ::ScriptContext::CustomReaderWriter(&ToLua, &FromLua))
                    ;
            }
        }
    };
	
	*/
}