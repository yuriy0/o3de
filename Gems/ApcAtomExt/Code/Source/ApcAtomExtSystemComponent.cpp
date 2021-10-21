#include "ApcAtomExtSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

// extensions functionality
#include <Passes/GenericCopyFrameBufferPass.h>
#include <Passes/GenericScreenSpaceBlurChildPass.h>
#include <Passes/GenericScreenSpaceBlurPass.h>

namespace ApcAtomExt
{
    void ApcAtomExtSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
			serialize->Class<ApcAtomExtSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
				ec->Class<ApcAtomExtSystemComponent>("ApcAtomExtSystemComponent", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    ApcAtomExtSystemComponent::ApcAtomExtSystemComponent()
    {
    }

    ApcAtomExtSystemComponent::~ApcAtomExtSystemComponent()
    {
    }

    void ApcAtomExtSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ApcAtomExtSystemComponentService"));
    }

    void ApcAtomExtSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ApcAtomExtSystemComponentService"));
    }

    void ApcAtomExtSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // Required for AZ::RPI::PassSystemInterface to be available in Activate
        required.push_back(AZ_CRC("RPISystem"));
    }

    void ApcAtomExtSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void ApcAtomExtSystemComponent::Activate()
    {
        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");
        if (passSystem)
        {
            using namespace AZ::Render;
            passSystem->AddPassCreator(AZ::Name("GenericCopyFrameBufferPass"), &GenericCopyFrameBufferPass::Create);
            passSystem->AddPassCreator(AZ::Name("GenericScreenSpaceBlurPass"), &GenericScreenSpaceBlurPass::Create);
            passSystem->AddPassCreator(AZ::Name("GenericScreenSpaceBlurChildPass"), &GenericScreenSpaceBlurChildPass::Create);

            m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler(
            [this, passSystem]() {
                passSystem->LoadPassTemplateMappings("Passes/ApcAtomExt_PassTemplates.azasset");
            });
            passSystem->ConnectEvent(m_loadTemplatesHandler);
        }
    }

    void ApcAtomExtSystemComponent::Deactivate()
    {
    }
}
