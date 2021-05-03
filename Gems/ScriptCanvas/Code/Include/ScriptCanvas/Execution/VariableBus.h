#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/variant.h>

namespace ScriptCanvas
{
    enum class VariableAccessError
    {
        StateError, NonExistentVariable, WrongTypeValue
    };

    struct VariableSetResult
    {
        AZ_TYPE_INFO(VariableSetResult, "{4CE2C3CE-FEEF-470C-9C17-EE7DE7329314}");

        constexpr VariableSetResult() = default;

        constexpr VariableSetResult(VariableAccessError err)
            : m_maybeError(err)
        {}

        constexpr operator bool() const
        {
            return AZStd::get_if<AZStd::monostate>(&m_maybeError) != nullptr;
        }

        AZStd::variant<AZStd::monostate, VariableAccessError> m_maybeError;
    };

    struct VariableGetResult
    {
        AZ_TYPE_INFO(VariableGetResult, "{95817951-5310-4A89-9C54-EE3067525756}");

        constexpr VariableGetResult() = default;

        template<class ValT, class = AZStd::enable_if_t<!AZStd::is_same_v<AZStd::decay_t<ValT>, VariableAccessError>>>
        explicit constexpr VariableGetResult(ValT&& val)
            : m_valueOrError(AZStd::in_place_index<0>, AZStd::forward<ValT>(val))
        {}

        constexpr VariableGetResult(VariableAccessError err)
            : m_valueOrError(err)
        {}

        constexpr operator bool() const
        {
            return AZStd::get_if<AZStd::any>(&m_valueOrError) != nullptr;
        }

        AZStd::variant<AZStd::any, VariableAccessError> m_valueOrError;
    };

    class VariableRuntimeRequests
        : public AZ::ComponentBus
    {
    public:
        /* Sets the value of a variable
        */
        virtual VariableSetResult SetVariable(AZStd::string_view name, const AZStd::any& value) = 0;

        /* Gets the value of a variable
        */
        virtual VariableGetResult GetVariable(AZStd::string_view name) = 0;

    //private:
    //    bool SetVariable_Behavior(AZStd::string_view name, const AZStd::any& value);
    //    AZStd::any GetVariable_Behavior(AZStd::string_view name);

    //    static void Reflect(AZ::ReflectContext*);
    };
    using VariableRuntimeRequestBus = AZ::EBus<VariableRuntimeRequests>;
}
