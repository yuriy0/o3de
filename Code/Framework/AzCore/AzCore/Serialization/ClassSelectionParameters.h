#pragma once

// Use of AZStd::visit with variant causes "C4100: 'this': unreferenced formal parameter" at invoke_traits.h:174 with MSVC2015
// It's unclear how this is possible; "this" is not a formal parameter, nor is the function in question a member function (compiler bug)?
#pragma warning( push )
#pragma warning( disable: 4100 )
#include <AzCore/std/containers/variant.h>
#pragma warning( pop )

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/functional.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Outcome/Outcome.h>

namespace AZ
{
    namespace Edit { namespace Attributes {
        const static AZ::Crc32 ClassSelectionParameters = AZ::Crc32("ClassSelectionParameters");
    }}

    /*
    Used by edit context to determine which classes are valid for a particular container data node.
    Can be used to implement sum types, or restrict which types a particular `AZStd::any' field may be given in editor.
    */
    class ClassSelectionParameters
    {
    private:
        template<class X, class... Ts>
        using all_equal = AZStd::conjunction<AZStd::is_same<AZStd::decay_t<Ts>, X>...>;

    public:
        using FilterFn = AZStd::function<bool(const AZ::SerializeContext::ClassData*)>;

        AZ_TYPE_INFO(ClassSelectionParameters, "{EE35F84F-2E26-45EA-872C-6909EEE73496}");

        // Indicates that any class can be selected
        ClassSelectionParameters();

        // Indicates that subclasses of `classId' can be selected. One of the given type ids must be non-null
        ClassSelectionParameters(const AZ::Uuid& classId_, const AZ::Uuid& typeId_);

        // Indicates that only exactly the specified classes can be selected.
        ClassSelectionParameters(const AZStd::vector<AZ::Uuid>& candidates);
        ClassSelectionParameters(AZStd::vector<AZ::Uuid>&& candidates);

        // Indicates that classes matching the filter can be selected
        ClassSelectionParameters(const FilterFn& filterFn);
        ClassSelectionParameters(FilterFn&& filterFn);
        template<class Functor, class = AZStd::enable_if_t< AZStd::function_traits<AZStd::decay_t<Functor>>::value >>
        ClassSelectionParameters(Functor&& filterFn);

        // Indicates that classes matching any given selection parameters can be selected
        static ClassSelectionParameters Union(const AZStd::vector<ClassSelectionParameters>& opts);
        static ClassSelectionParameters Union(AZStd::vector<ClassSelectionParameters>&& opts);
        template<class... Params, class = AZStd::enable_if_t<all_equal<ClassSelectionParameters, Params...>::value>>
        static ClassSelectionParameters Union(Params&&... opts);

        // Indicates that classes matching all given selection parameters can be selected
        static ClassSelectionParameters Intersection(const AZStd::vector<ClassSelectionParameters>& opts);
        static ClassSelectionParameters Intersection(AZStd::vector<ClassSelectionParameters>&& opts);
        template<class... Params, class = AZStd::enable_if_t<all_equal<ClassSelectionParameters, Params...>::value>>
        static ClassSelectionParameters Intersection(Params&&... opts);

        // Runs the query. Returns either a non-empty vector of the matching class datums, or an error describing why it the query failed.
        AZ::Outcome<AZStd::vector<const AZ::SerializeContext::ClassData*>, AZStd::string> GetClasses(AZ::SerializeContext& sc) const;

    private:
        struct AnyClassT{};
        using SubclassT = AZStd::pair<AZ::Uuid, AZ::Uuid>;
        using SpecifiedClassesT = AZStd::vector<AZ::Uuid>;
        using FilteredClassesT = FilterFn;
        struct UnionT : public AZStd::vector<ClassSelectionParameters> {
            using base_t = AZStd::vector<ClassSelectionParameters>;
            template<class... Args>
            UnionT(Args&&... args) : base_t(AZStd::forward<Args>(args)...) {}
        };
        struct IntersectionT : public AZStd::vector<ClassSelectionParameters> {
            using base_t = AZStd::vector<ClassSelectionParameters>;
            template<class... Args>
            IntersectionT(Args&&... args) : base_t(AZStd::forward<Args>(args)...) {}
        };

        struct forwarding_constructor_t{};
        template<class... Args>
        ClassSelectionParameters(forwarding_constructor_t, Args&&... args);

        using ClassSet = AZStd::unordered_set<const AZ::SerializeContext::ClassData*>;
        void GetClassesImpl(AZ::SerializeContext& context, ClassSet& out) const;

        AZStd::string EmptySelectionError(AZ::SerializeContext& sc) const;

        using Impl = AZStd::variant<AnyClassT, SubclassT, SpecifiedClassesT, FilteredClassesT, UnionT, IntersectionT>;

        Impl m_impl;
    };

    template<class Functor, class>
    ClassSelectionParameters::ClassSelectionParameters(Functor&& filterFn)
        : m_impl(AZStd::in_place_type_t<FilteredClassesT>{}, AZStd::forward<Functor>(filterFn))
    {
        static_assert(AZStd::is_invocable_r<bool, AZStd::decay_t<Functor>, const AZ::SerializeContext::ClassData*>::value,
                      "ClassSelectionParameters \"filtered classes\" constructor requires functor with signature `bool(const AZ::SerializeContext::ClassData*)`"
                      );
    }

    template<class... Params, class>
    ClassSelectionParameters ClassSelectionParameters::Union(Params&&... opts)
    {
        return ClassSelectionParameters(forwarding_constructor_t{}, AZStd::in_place_type_t<UnionT>{}, UnionT::base_t{ opts... });
    }

    template<class... Params, class>
    ClassSelectionParameters ClassSelectionParameters::Intersection(Params&&... opts)
    {
        return ClassSelectionParameters(forwarding_constructor_t{}, AZStd::in_place_type_t<IntersectionT>{}, IntersectionT::base_t{ opts... });
    }

    // Forwarding constructor
    template<class... Args>
    ClassSelectionParameters::ClassSelectionParameters(forwarding_constructor_t, Args&&... args)
        : m_impl(AZStd::forward<Args>(args)...)
    {}
}