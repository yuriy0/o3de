#pragma once

#include <AzCore/std/containers/variant.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ {
    template<class... Ts>
    struct OnDemandReflection<AZStd::variant<Ts...>> {
        using ThisType = AZStd::variant<Ts...>;

        template<class T>
        static bool CustomConstructorCheck(ThisType* thisPtr, AZ::ScriptDataContext& dc) {
            T val;
            // HACK! IsClass doesn't work for non-classes
            if (dc.IsClass<T>(0) && dc.ReadArg<T>(0, val)) {
                new(thisPtr) ThisType(AZStd::move(val));
                return true;
            } else {
                return false;
            }
        }

        static void CustomConstructor(ThisType* thisPtr, AZ::ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                bool ret = false;
                int _[] = { 0, ( ret |= CustomConstructorCheck<Ts>(thisPtr, dc), 0 ) ... };
            }
        }

        static void ToLua(lua_State* lua, AZ::BehaviorValueParameter& param) {
            ThisType& value = *param.GetAsUnsafe<ThisType>();
            AZStd::visit([&](const auto& v) {
                using T = AZStd::decay_t<decltype(v)>;
                AZ::ScriptValue<T>::StackPush(lua, v);
            }, value);
        }

        template<class T>
        static bool FromLuaCheck(lua_State* lua, int stackIndex, AZ::BehaviorValueParameter& outVal, void* storage) {
            // HACK! LuaIsClass doesn't work for non-classes
            if (AZ::Internal::LuaIsClass(lua, stackIndex, &AZ::AzTypeInfo<T>::Uuid())) {
                outVal.Set(new (storage) ThisType( AZ::ScriptValue<T>::StackRead(lua, stackIndex) ));
                return true;
            } else {
                return false;
            }
        }

        static bool FromLua(lua_State* lua, int stackIndex, AZ::BehaviorValueParameter& value, AZ::BehaviorClass*, AZ::ScriptContext::StackVariableAllocator* stackTempAllocator) {
            bool didAllocation = false;
            void* thisPtr = nullptr;
            if (value.m_value) {
                thisPtr = value.m_value;
            } else if (stackTempAllocator != nullptr) {
                thisPtr = stackTempAllocator->allocate(sizeof(ThisType), AZStd::alignment_of<ThisType>::value);
                if (thisPtr == nullptr) {
                    AZ_Assert(false, "variant::FromLua failed to allocate temp memory!");
                    return false;
                }
                didAllocation = true;
            } else {
                AZ_Assert(false, "variant::FromLua called without temp storage and null allocator!");
                return false;
            }

            bool ret = false;
            int _[] = { 0, ( ret |= FromLuaCheck<Ts>(lua, stackIndex, value, thisPtr), 0 ) ... };

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
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::Ignore, true)

                    ->template Constructor<ThisType>()
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &CustomConstructor)

                    ->Attribute(AZ::Script::Attributes::ReaderWriterOverride, AZ::ScriptContext::CustomReaderWriter(&ToLua, &FromLua))
                    ;
            }
        }
    };
}