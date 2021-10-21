#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IBoneRenamingRule
                : public IRule
            {
            public:
                AZ_RTTI(IBoneRenamingRule, "{1301CD8F-8E0D-4564-B263-AE653BEAFF81}", IRule);

                virtual ~IBoneRenamingRule() override = default;

                using BoneRenamer = AZStd::function<AZStd::string(const AZStd::string&)>;
                virtual BoneRenamer GetBoneRenamer() const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
