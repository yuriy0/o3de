#include <AzCore/Serialization/ClassSelectionParameters.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/match.h>

namespace AZ
{
    // AnyClass
    ClassSelectionParameters::ClassSelectionParameters()
        : m_impl(AZStd::in_place_type_t<AnyClassT>{})
    {}

    // Subclass
    ClassSelectionParameters::ClassSelectionParameters(const AZ::Uuid& classId, const AZ::Uuid& typeId)
        : m_impl(AZStd::in_place_type_t<SubclassT>{}, classId, typeId)
    {
        AZ_Assert(!classId.IsNull() || !typeId.IsNull(), __FUNCTION__ " - fatal, both types are null");
    }

    // SpecifiedClasses
    ClassSelectionParameters::ClassSelectionParameters(const AZStd::vector<AZ::Uuid>& candidates)
        : m_impl(AZStd::in_place_type_t<SpecifiedClassesT>{}, candidates)
    {}
    ClassSelectionParameters::ClassSelectionParameters(AZStd::vector<AZ::Uuid>&& candidates)
        : m_impl(AZStd::in_place_type_t<SpecifiedClassesT>{}, AZStd::move(candidates))
    {}

    // FilteredClasses
    ClassSelectionParameters::ClassSelectionParameters(const ClassSelectionParameters::FilterFn& filterFn)
        : m_impl(AZStd::in_place_type_t<FilteredClassesT>{}, filterFn)
    {}
    ClassSelectionParameters::ClassSelectionParameters(ClassSelectionParameters::FilterFn&& filterFn)
        : m_impl(AZStd::in_place_type_t<FilteredClassesT>{}, AZStd::move(filterFn))
    {}

    // Union
    ClassSelectionParameters ClassSelectionParameters::Union(const AZStd::vector<ClassSelectionParameters>& opts)
    {
        return ClassSelectionParameters(forwarding_constructor_t{}, AZStd::in_place_type_t<UnionT>{}, opts);
    }
    ClassSelectionParameters ClassSelectionParameters::Union(AZStd::vector<ClassSelectionParameters>&& opts)
    {
        return ClassSelectionParameters(forwarding_constructor_t{}, AZStd::in_place_type_t<UnionT>{}, AZStd::move(opts));
    }

    // Intersection
    ClassSelectionParameters ClassSelectionParameters::Intersection(const AZStd::vector<ClassSelectionParameters>& opts)
    {
        return ClassSelectionParameters(forwarding_constructor_t{}, AZStd::in_place_type_t<IntersectionT>{}, opts);
    }
    ClassSelectionParameters ClassSelectionParameters::Intersection(AZStd::vector<ClassSelectionParameters>&& opts)
    {
        return ClassSelectionParameters(forwarding_constructor_t{}, AZStd::in_place_type_t<IntersectionT>{}, AZStd::move(opts));
    }

    AZ::Outcome<AZStd::vector<const AZ::SerializeContext::ClassData*>, AZStd::string> ClassSelectionParameters::GetClasses(AZ::SerializeContext& context) const
    {
        ClassSet out;
        GetClassesImpl(context, out);
        if (out.empty()) { return AZ::Failure(EmptySelectionError(context)); }

        AZStd::vector<const AZ::SerializeContext::ClassData*> vout;
        vout.reserve(out.size());
        for (const auto& cl : out) { vout.push_back(cl); };

        return AZ::Success(vout);
    }

    template<class S0, class S1>
    void set_intersection(S0& s0, const S1& s1) {
        for (auto it = s0.begin(); it != s0.end(); ) {
            if (s1.find(*it) == s1.end()) { 
                it = s0.erase(it);
            } else { 
                it++;
            }
        }
    }

    void ClassSelectionParameters::GetClassesImpl(AZ::SerializeContext& context, ClassSet& out) const
    {
        AZStd::match(
            m_impl,
            [&](const AnyClassT&)
            {
                context.EnumerateAll(
                    [&out](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool {
                        out.insert(classData);
                        return true;
                    }
                );
            },
            [&](const SubclassT& subclass)
            {
                context.EnumerateDerived(
                    [&out](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool {
                        out.insert(classData);
                        return true;
                    },
                    subclass.first,
                    subclass.second
                );
            },
            [&](const SpecifiedClassesT& specifiedClasses)
            {
                for (auto& typ : specifiedClasses) {
                    if (auto* cd = context.FindClassData(typ)) {
                        out.insert(cd);
                    }
                }
            },
            [&](const FilteredClassesT& filteredClasses)
            {
                context.EnumerateAll(
                    [&out, &filteredClasses](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool {
                        if (filteredClasses(classData)) out.insert(classData);
                        return true;
                    },
                    /* includeGenerics = */ true
                );
            },
            [&](const UnionT& opts)
            {
                for (const auto& opt : opts) { opt.GetClassesImpl(context, out); };
            },
            [&](const IntersectionT& opts)
            {
                auto it = opts.begin();
                const auto end = opts.end();
                if (it == end) return;

                ClassSet base;
                it->GetClassesImpl(context, base);
                ++it;

                ClassSet opt_s;
                for (; it != end; ++it) {
                    it->GetClassesImpl(context, opt_s);
                    set_intersection(base, opt_s);
                    opt_s.clear();
                };

                for (const auto& cl : base) { out.insert(cl); }
            }
        );
    }

    AZStd::string ClassSelectionParameters::EmptySelectionError(AZ::SerializeContext& context) const
    {
        const auto recurse = [&](const char* fmt, const AZStd::vector<ClassSelectionParameters>& opts)
        {
            AZStd::string opts_s;
            for (auto& opt : opts) {
                opts_s += opt.EmptySelectionError(context) + "\n";
            }
            return AZStd::string::format(fmt, opts_s.c_str());
        };

        return AZStd::match(
            m_impl,
            [&](const AnyClassT&)
            {
                return AZStd::string::format("Internal error - SerializeContext has no serialized types");
            },
            [&](const SubclassT& subclass)
            {
                const AZ::SerializeContext::ClassData* classData = context.FindClassData(subclass.first);
                const char* className = classData ?
                    (classData->m_editData ? classData->m_editData->m_name : classData->m_name)
                    : "<unknown>";

                return AZStd::string::format("No classes could be found that derive from \"%s\".", className);
            },
            [&](const SpecifiedClassesT& specifiedClasses)
            {
                if (specifiedClasses.empty())
                {
                    return AZStd::string("Specified class list is empty");
                }
                else
                {
                    AZStd::string cl;
                    for (auto& typ : specifiedClasses) {
                        cl += typ.ToString<AZStd::string>() + ", ";
                    }

                    return AZStd::string::format("None of the classes {%s} are reflected to the serialize context", cl.c_str());
                }
            },
            [&](const FilteredClassesT&)
            {
                return AZStd::string::format("Class filter matched nothing");
            },
            [&](const UnionT& opts)
            {
                return recurse("Class filter matched nothing because all of the union of class filters matched nothing:%s", opts);
            },
            [&](const IntersectionT& opts)
            {
                return recurse("Class filter matched nothing because no classes match all of the intersection class filters:%s", opts);
            }
        );
    }
}


