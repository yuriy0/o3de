
#pragma once

#include <AzCore/Component/Component.h>

#include <Deadhaus_Sonata/Deadhaus_SonataBus.h>

namespace Deadhaus_Sonata
{
    class Deadhaus_SonataSystemComponent
        : public AZ::Component
        , protected Deadhaus_SonataRequestBus::Handler
    {
    public:
        AZ_COMPONENT(Deadhaus_SonataSystemComponent, "{3a55fb5d-81ca-4a00-90b8-62087b8975b6}");

        static void Reflect(AZ::ReflectContext* context);

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
