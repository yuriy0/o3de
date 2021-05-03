#pragma once

#include "any.h"

namespace AZStd { 
    namespace Internal {
        template<class Type, bool = AZStd::is_arithmetic<Type>::value>
        struct any_generic_cast_t;

        template<class Type>
        struct any_generic_cast_t<Type, true> {
            inline static bool call(const AZStd::any& anyVal, Type& realVal) {
                return AZStd::any_numeric_cast(&anyVal, realVal);
            }
        };

        template<class Type>
        struct any_generic_cast_t<Type, false> {
            inline static bool call(const AZStd::any& anyVal, Type& realVal) {
                if (const Type* realVal_ = AZStd::any_cast<Type>(&anyVal)) {
                    realVal = *realVal_;
                    return true;
                } else {
                    return false;
                }
            }
        };
    }

    /* A version of any_cast which calls any_numeric_cast for numeric types
    */
    template<class Type>
    bool any_generic_cast(const AZStd::any& anyVal, Type& realVal) { 
        return Internal::any_generic_cast_t<Type>::call(anyVal, realVal);
    }
}