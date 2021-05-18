#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "Deadhaus_SonataSystemComponent.h"

namespace Deadhaus_Sonata
{
    
    class Deadhaus_SonataModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(Deadhaus_SonataModule, "{F5D84B1E-0200-402F-AED5-345FCC695CD1}", AZ::Module);
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
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Deadhaus_Sonata_0ccc66f597594b50814669ffdaf584b3, Deadhaus_Sonata::Deadhaus_SonataModule)

