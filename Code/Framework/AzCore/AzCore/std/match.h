#pragma once

#include <AzCore/std/containers/variant.h>
#include <AzCore/std/typetraits/decay.h>
#include <AzCore/std/utils.h>

namespace AZStd {
    template<class... Ts> struct overload_t : Ts...
    {
        using Ts::operator()...;
    };

    template<class... Ts>
    using overload_t_decayed = overload_t<AZStd::decay_t<Ts>...>;

    template<class... MatchFns>
    inline constexpr decltype(auto) overload(MatchFns&&... matchFns) {
        return overload_t_decayed<MatchFns&&...>{AZStd::forward<MatchFns>(matchFns)...};
    }

    template<class Variant, class ...MatchFns>
    inline constexpr decltype(auto) match(Variant&& variant, MatchFns&& ...matchFns)
    {
        using visitor_t = overload_t_decayed<MatchFns&&...>;
        return visit<visitor_t, Variant>(visitor_t{AZStd::forward<MatchFns>(matchFns)...}, AZStd::forward<Variant>(variant));
    }

    // This type transforms a functor `Base' into another functor which behaves like base but casts its return
    // value to a single target type, i.e. the return type of `target_type_cast<R, Base>::operator()' 
    // does not depends on its argument types, even if the return type of `Base::operator()' does.
    template<class R, class Base>
    struct target_type_cast : Base
    {
        template<class... Args>
        AZ_FORCE_INLINE auto operator() (Args&&... args)
            -> decltype( static_cast<R>( AZStd::declval<Base>()(AZStd::forward<Args>(args)...)) )
        {
            // We would prefer to have the oneliner:
            //   static_cast<R>(Base::operator()(AZStd::forward<Args>(args)...));
            // but this gives ICE on MSVC 15.9.28307.1064 (Sept 3 2020)
            Base* pObj = this;
            Base& obj = *pObj;
            return static_cast<R>(obj(AZStd::forward<Args>(args)...));
        }

        template<class... Args>
        AZ_FORCE_INLINE auto operator() (Args&&... args) const
            -> decltype( static_cast<R>( AZStd::declval<const Base>()(AZStd::forward<Args>(args)...)) )
        {
            const Base* pObj = this;
            const Base& obj = *pObj;
            return static_cast<R>(obj(AZStd::forward<Args>(args)...));
        }

        template<class... Args>
        target_type_cast(Args&&... args)
            : Base(AZStd::forward<Args>(args)...)
        {}
    };

    template<class R, class Variant, class ...MatchFns>
    inline constexpr R match(Variant&& variant, MatchFns&& ...matchFns)
    {
        using overloaded_t = overload_t<AZStd::decay_t<MatchFns&&>...>;
        using visitor_t = target_type_cast<R, overloaded_t>;
        return visit<R, visitor_t, Variant>(
            visitor_t(overloaded_t{AZStd::forward<MatchFns>(matchFns)...}),
            AZStd::forward<Variant>(variant));
    }
}
