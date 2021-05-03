#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "ApcAtomExtSystemComponent.h"

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

AZ_DECLARE_MODULE_CLASS(ApcAtomExtModule_b199505e84a344c389606b66bab8226b, ApcAtomExt::ApcAtomExtModule)
