#include <SceneAPIExt/Rules/IMotionZeroRule.h>
#include <AzCore/Math/Quaternion.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            bool IMotionZeroRule::CreateOffsetTransform(AZ::SceneAPI::DataTypes::MatrixType& tm) const {
                if (!DoZeroTranslation() && !DoZeroRotation() && !DoZeroScale()) return false;

                if (!DoZeroTranslation()) {
                    tm.SetTranslation(AZ::Vector3::CreateZero());
                }
                if (!DoZeroRotation()) {
                    tm.SetRotationPartFromQuaternion(AZ::Quaternion::CreateIdentity());
                }
                if (!DoZeroScale()) {
                    tm.ExtractScale();
                }

                return true;
            }

        }  // Rule
    }  // Pipeline
}  // EMotionFX