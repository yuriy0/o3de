#pragma once

#include <AzCore/Component/Component.h>


namespace Deadhaus_Sonata
{
    class Deadhaus_SonataSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(Deadhaus_SonataSystemComponent, "{83FFD45D-2C42-4212-91BE-AC351F88BA50}");

        static void Reflect(AZ::ReflectContext* context);

        Deadhaus_SonataSystemComponent();
        ~Deadhaus_SonataSystemComponent();

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // Deadhaus_SonataRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    };
}
