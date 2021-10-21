#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "ApcAtomExtSystemComponent.h"
#include "Components/MaterialPropertyRandomizerComponent.h"

namespace ApcAtomExt
{
    class ApcAtomExtModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ApcAtomExtModule, "{502FA3B7-E504-40F6-94BD-3B26E2D10F33}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ApcAtomExtModule, AZ::SystemAllocator, 0);

        ApcAtomExtModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                ApcAtomExtSystemComponent::CreateDescriptor(),
                MaterialPropertyRandomizerComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ApcAtomExtSystemComponent>(),
            };
        }
    };
}

AZ_DECLARE_MODULE_CLASS(Gem_ApcAtomExt, ApcAtomExt::ApcAtomExtModule)
