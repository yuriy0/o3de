/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzSceneDef.h>
#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IBoneRenamingRule.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class BoneRenamingRule
                : public AZ::SceneAPI::DataTypes::IBoneRenamingRule
            {
            public:
                AZ_RTTI(BoneRenamingRule, "{8919BCEC-C336-44B3-8ACB-85A6E6AA3997}", IBoneRenamingRule);
                AZ_CLASS_ALLOCATOR_DECL;

                BoneRenamingRule() = default;
                ~BoneRenamingRule() override = default;

                static void Reflect(AZ::ReflectContext * context);

                using BoneRenamer = AZStd::function<AZStd::string(const AZStd::string&)>;
                BoneRenamer GetBoneRenamer() const;
            protected:

                struct RegexReplacement
                {
                    AZ_TYPE_INFO(RegexReplacement, "{2930505E-EC4E-4FFD-BB1B-63C6BFA3507A}");

                    AZStd::string m_find;
                    AZStd::string m_replace;
                };

                AZStd::vector<RegexReplacement> m_replacements;
            };
        } // Rule
    } // Pipeline
} // EMotionFX

