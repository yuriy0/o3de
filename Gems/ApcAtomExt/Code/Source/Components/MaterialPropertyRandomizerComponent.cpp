#include "MaterialPropertyRandomizerComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/std/VariantReflection.inl>
#include <AzCore/Script/std/VariantReflection.inl>
#include <AzCore/Asset/AssetSerializer.h>

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <random>

namespace ApcAtomExt
{
    void MaterialPropertyValue::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialPropertyValue>()
                ->Field("m_value", &MaterialPropertyValue::m_value)
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MaterialPropertyValue>("MaterialPropertyValue", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(0, &MaterialPropertyValue::m_value, "", "")
                    ;
            }
        }

        if (auto behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<MaterialPropertyValue>("MaterialPropertyValue")
                ->WrappingMember<value_type*>(&MaterialPropertyValue::unwrap)
                ->Property("value", BehaviorValueProperty(&MaterialPropertyValue::m_value))
                ;
        }
    }

    void MaterialPropertyRandomizerComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        MaterialPropertyValue::Reflect(context);
        decltype(randomChoices)::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialPropertyRandomizerComponentConfig>()
                ->Version(0)
                ->Field("propertyName", &MaterialPropertyRandomizerComponentConfig::propertyName)
                ->Field("submaterials", &MaterialPropertyRandomizerComponentConfig::submaterials)
                ->Field("sameChoiceForEachSubmaterial", &MaterialPropertyRandomizerComponentConfig::sameChoiceForEachSubmaterial)
                ->Field("randomChoices", &MaterialPropertyRandomizerComponentConfig::randomChoices)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MaterialPropertyRandomizerComponentConfig>("MaterialPropertyRandomizerComponentConfig", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(0, &MaterialPropertyRandomizerComponentConfig::propertyName,
                        "Property name", "The script name of the material property to randomize")

                    ->DataElement(0, &MaterialPropertyRandomizerComponentConfig::submaterials,
                        "Submaterials", "The submaterial IDs on which to randomize properties, or if empty, all submaterials")

                    ->DataElement(0, &MaterialPropertyRandomizerComponentConfig::sameChoiceForEachSubmaterial,
                        "Same choice for each submaterial?", "If true, pick the same random choice for each submaterial")

                    ->DataElement(0, &MaterialPropertyRandomizerComponentConfig::randomChoices,
                        "Random choices", "The random choices")
                    ;
            }
        }
    }

    void MaterialPropertyRandomizerComponent::Reflect(AZ::ReflectContext* context)
    {
        MaterialPropertyRandomizerComponentConfig::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialPropertyRandomizerComponent, AZ::Component>()
                ->Version(0)
                ->Field("m_config", &MaterialPropertyRandomizerComponent::m_config)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MaterialPropertyRandomizerComponent>("MaterialPropertyRandomizerComponent", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))

                    ->DataElement(0, &MaterialPropertyRandomizerComponent::m_config, "", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void MaterialPropertyRandomizerComponent::Activate()
    {
        using namespace AZ::Render;
        const auto entity = GetEntityId();

        // Collect specified submaterial IDs
        AZStd::vector<MaterialAssignmentId> submaterials;
        MaterialReceiverRequestBus::Event(entity, [&](MaterialReceiverRequestBus::Events* materialInterface)
        {
            for (const auto& submaterialName : m_config.submaterials)
            {
                // Handles both LOD0 and all LODs (in case of LOD materials)
                submaterials.emplace_back(materialInterface->FindMaterialAssignmentId(0, submaterialName));
                submaterials.emplace_back(materialInterface->FindMaterialAssignmentId(MaterialAssignmentId::NonLodIndex, submaterialName));
            }
        });

        // If no submaterial IDs specified, get all submaterial ids
        if (submaterials.empty())
        {
            MaterialAssignmentMap originalMatAssignments;
            MaterialComponentRequestBus::EventResult(originalMatAssignments, entity, &MaterialComponentRequestBus::Events::GetOriginalMaterialAssignments);
            for (auto& matAssignment : originalMatAssignments)
            {
                submaterials.emplace_back(AZStd::move(matAssignment.first));
            }
        }

        std::random_device rng;
        const auto ChooseRandomProperty = [&]() -> AZStd::any
        {
            const auto& val = m_config.randomChoices.GetOptions()[m_config.randomChoices.GetRandomOption(rng)].val.m_value;
            return AZStd::visit([](const auto& tval) { return AZStd::any(tval); }, val);
        };

        // Get a random choice
        AZStd::function<const AZStd::any&(size_t)> GetRandomChoiceForIndex;
        if (m_config.sameChoiceForEachSubmaterial)
        {
            GetRandomChoiceForIndex =
                [
                    choice = ChooseRandomProperty()
                ]
            (size_t) -> const AZStd::any&
            {
                return choice;
            };
        }
        else
        {
            GetRandomChoiceForIndex =
                [
                    choices = [&]()
                    {
                        AZStd::vector<AZStd::any> cs;
                        AZStd::generate_n(AZStd::inserter(cs, cs.begin()), submaterials.size(), ChooseRandomProperty);
                        return cs;
                    }()
                ]
            (size_t i) -> const AZStd::any&
            {
                return choices[i];
            };
        }

        // Assign randomized properties
        MaterialComponentRequestBus::Event(entity, [&](MaterialComponentRequestBus::Events* materialInterface)
        {
            for (size_t i = 0; i < submaterials.size(); ++i)
            {
                materialInterface->SetPropertyOverride(submaterials[i], m_config.propertyName, GetRandomChoiceForIndex(i));
            }
        });

        // Destroys self...
        OnDoneRandomizing();
    }

    void MaterialPropertyRandomizerComponent::Deactivate()
    {
    }

    void MaterialPropertyRandomizerComponent::OnDoneRandomizing()
    {
        if (GetEntity()->GetState() == AZ::Entity::State::Activating)
        {
            AZ::TickBus::QueueFunction([this]() { OnDoneRandomizing(); });
            return;
        }

        if (GetEntity()->Modify({ this }, {}))
        {
            delete this;
        }
        else
        {
            AZ_Warning("MaterialPropertyRandomizerComponent", false, "Failed to remove this component from its entity");
        }
    }
}
