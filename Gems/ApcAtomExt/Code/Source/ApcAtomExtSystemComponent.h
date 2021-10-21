#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

namespace ApcAtomExt
{
    class ApcAtomExtSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ApcAtomExtSystemComponent, "{801D4FDC-7306-4A9A-93F4-64D3FEB99D80}");

        static void Reflect(AZ::ReflectContext* context);

        ApcAtomExtSystemComponent();
        ~ApcAtomExtSystemComponent();

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
    };
}
