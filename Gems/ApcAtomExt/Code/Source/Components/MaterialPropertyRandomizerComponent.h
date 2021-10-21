#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

// #include <ApcUtilities/DataStructures/DiscreteDistribution.h>

namespace ApcAtomExt
{
    struct MaterialPropertyValue
    {
        AZ_TYPE_INFO(MaterialPropertyValue, "{35ABBA2C-5313-42DB-94C4-0EABC2A2BACC}");
        static void Reflect(AZ::ReflectContext* context);

        using value_type = AZStd::variant<
            bool, int32_t, uint32_t, float, AZ::Vector2, AZ::Vector3, AZ::Vector4, AZ::Color,
            AZ::Data::Asset<AZ::RPI::StreamingImageAsset>
        >;

        value_type* unwrap()
        {
            return &m_value;
        }

        value_type m_value;
    };

    struct MaterialPropertyRandomizerComponentConfig
    {
        AZ_TYPE_INFO(MaterialPropertyRandomizerComponentConfig, "{2F6AFD45-60EC-4694-B698-EDEAEA75DE7A}");
        static void Reflect(AZ::ReflectContext* context);

        AZStd::string propertyName;
        AZStd::vector<AZStd::string> submaterials;
        bool sameChoiceForEachSubmaterial = false;
        // ApcUtilities::DiscreteDistribution<MaterialPropertyValue> randomChoices;
    };

    class MaterialPropertyRandomizerComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MaterialPropertyRandomizerComponent, "{6DAF9822-826C-40A4-A1AE-B5D4CE131A11}");

        MaterialPropertyRandomizerComponent() = default;
        virtual ~MaterialPropertyRandomizerComponent() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("MaterialProviderService"));
        }

    private:
        /////////////////////////////////////////////////////////////
        //AZ::Component
        void Activate() override;
        void Deactivate() override;
        /////////////////////////////////////////////////////////////

        void OnDoneRandomizing();

        // Reflected data
        MaterialPropertyRandomizerComponentConfig m_config;
    };
}
