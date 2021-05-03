#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPIExt/Rules/MotionZeroRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(MotionZeroRule, AZ::SystemAllocator, 0);

            MotionZeroRule::MotionZeroRule()
                : m_zeroTranslation(true)
                , m_zeroRotation(true)
                , m_zeroScale(true)
            {}

            bool MotionZeroRule::DoZeroTranslation() const {
                return m_zeroTranslation;
            }

            bool MotionZeroRule::DoZeroRotation() const {
                return m_zeroRotation;
            }

            bool MotionZeroRule::DoZeroScale() const {
                return m_zeroScale;
            }

            void MotionZeroRule::Reflect(AZ::ReflectContext* context)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context)) {
                    serializeContext->Class<IMotionZeroRule, AZ::SceneAPI::DataTypes::IRule>()
                        ->Version(1);

                    serializeContext->Class<MotionZeroRule, IMotionZeroRule>()
                        ->Version(1)
                        ->Field("m_zeroTranslation", &MotionZeroRule::m_zeroTranslation)
                        ->Field("m_zeroRotation", &MotionZeroRule::m_zeroRotation)
                        ->Field("m_zeroScale", &MotionZeroRule::m_zeroScale)
                        ;

                    if (auto editContext = serializeContext->GetEditContext()) {
                        editContext->Class<MotionZeroRule>("Zero motion", "Sets some components of the motion at start frame to zero")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                            ->DataElement(AZ::Edit::UIHandlers::Default, &MotionZeroRule::m_zeroTranslation, "Zero translation?", "If true, the translation at the start frame will be set to zero")

                            ->DataElement(AZ::Edit::UIHandlers::Default, &MotionZeroRule::m_zeroRotation, "Zero rotation?", "If true, the rotation at the start frame will be set to the identity")

                            ->DataElement(AZ::Edit::UIHandlers::Default, &MotionZeroRule::m_zeroScale, "Zero scale?", "If true, the scale at the start frame will be set to the identity")
                            ;
                    }
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX