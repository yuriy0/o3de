
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "Deadhaus_SonataSystemComponent.h"

namespace Deadhaus_Sonata
{
    class Deadhaus_SonataModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(Deadhaus_SonataModule, "{4b6814b7-9788-49bf-a338-207ed8baa1b5}", AZ::Module);
        AZ_CLASS_ALLOCATOR(Deadhaus_SonataModule, AZ::SystemAllocator, 0);

        Deadhaus_SonataModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                Deadhaus_SonataSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<Deadhaus_SonataSystemComponent>(),
            };
        }
    };
}// namespace Deadhaus_Sonata

AZ_DECLARE_MODULE_CLASS(Project_Deadhaus_Sonata, Deadhaus_Sonata::Deadhaus_SonataModule)
