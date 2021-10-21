#include "BoneRenamingRule.h"
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BoneRenamingRule.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/std/string/regex.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(BoneRenamingRule, AZ::SystemAllocator, 0);

            void BoneRenamingRule::Reflect(AZ::ReflectContext* context)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<RegexReplacement>()
                        ->Version(1)
                        ->Field("m_find", &RegexReplacement::m_find)
                        ->Field("m_replace", &RegexReplacement::m_replace)
                        ;

                    serializeContext->Class<IBoneRenamingRule, AZ::SceneAPI::DataTypes::IRule>()
                        ->Version(1);

                    serializeContext->Class<BoneRenamingRule, IBoneRenamingRule>()
                        ->Version(1)
                        ->Field("m_replacements", &BoneRenamingRule::m_replacements)
                        ;

                    if (auto editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<RegexReplacement>(
                            "Regex replacement", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &RegexReplacement::m_find, "Find", "The string to search for in bone names")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &RegexReplacement::m_replace, "Replace", "The string with which to replace found search strings")
                            ;

                        editContext->Class<BoneRenamingRule>(
                            "Bone Renaming Rule (ADVANCED)", "Rename the bones in the skeleton")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default,
                                &BoneRenamingRule::m_replacements,
                                "Replacements",
                                "")
                            ;
                    }
                }
            }

            AZStd::function<AZStd::string(const AZStd::string&)> BoneRenamingRule::GetBoneRenamer() const
            {
                static const auto CompileRegexes = [](const AZStd::vector<RegexReplacement>& replacements)
                {
                    AZStd::vector<AZStd::regex> res;
                    res.reserve(replacements.size());

                    for (const auto& replacement : replacements)
                    {
                        res.emplace_back(replacement.m_find);
                        const AZStd::regex& re = res.back();
                        if (!re.Valid())
                        {
                            AZ_Warning("BoneRenamingRule", false, "String '%s' is not a valid regular expression: %s", replacement.m_find.c_str(), re.GetError());
                        }
                    }

                    return res;
                };

                return [this, res = CompileRegexes(m_replacements)](const AZStd::string& str)
                {
                    AZStd::string result = str;
                    for (size_t i = 0; i < m_replacements.size(); ++i)
                    {
                        const AZStd::regex& findRe = res[i];
                        if (findRe.Valid())
                        {
                            const AZStd::string replStr = m_replacements[i].m_replace;
                            result = AZStd::regex_replace(str, findRe, replStr);
                        }
                    }
                    return result;
                };
            }
        }
    }
}
