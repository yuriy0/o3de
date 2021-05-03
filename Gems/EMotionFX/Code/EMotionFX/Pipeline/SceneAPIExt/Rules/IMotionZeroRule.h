#pragma once

#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Transform.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class IMotionZeroRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(IMotionZeroRule, "{1740B400-BE14-4965-8A76-9DAA1515012F}", AZ::SceneAPI::DataTypes::IRule);

                ~IMotionZeroRule() override = default;

                virtual bool DoZeroTranslation() const = 0;
                virtual bool DoZeroRotation() const = 0;
                virtual bool DoZeroScale() const = 0;

                /*
                    Based on configured options, creates a transform suitable to pre-multiply with keyframe transform data to implement the
                    configured zeroing.
                    Returns true iff at least one option is `true', otherwise there is nothing to do at all.
                */
                bool CreateOffsetTransform(AZ::SceneAPI::DataTypes::MatrixType& tm) const;
            };
        }  // Rule
    }  // Pipeline
}  // EMotionFX
