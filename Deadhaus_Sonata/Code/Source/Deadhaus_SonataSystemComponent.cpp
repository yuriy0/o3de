
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include "Deadhaus_SonataSystemComponent.h"

namespace Deadhaus_Sonata
{
    void Deadhaus_SonataSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Deadhaus_SonataSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<Deadhaus_SonataSystemComponent>("Deadhaus_Sonata", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void Deadhaus_SonataSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("Deadhaus_SonataService"));
    }

    void Deadhaus_SonataSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("Deadhaus_SonataService"));
    }

    void Deadhaus_SonataSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void Deadhaus_SonataSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void Deadhaus_SonataSystemComponent::Init()
    {
    }

    void Deadhaus_SonataSystemComponent::Activate()
    {
        Deadhaus_SonataRequestBus::Handler::BusConnect();
    }

    void Deadhaus_SonataSystemComponent::Deactivate()
    {
        Deadhaus_SonataRequestBus::Handler::BusDisconnect();
    }
}
