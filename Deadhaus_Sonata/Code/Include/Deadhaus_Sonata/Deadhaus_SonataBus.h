
#pragma once

#include <AzCore/EBus/EBus.h>

namespace Deadhaus_Sonata
{
    class Deadhaus_SonataRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Put your public methods here
    };
    using Deadhaus_SonataRequestBus = AZ::EBus<Deadhaus_SonataRequests>;
} // namespace Deadhaus_Sonata
