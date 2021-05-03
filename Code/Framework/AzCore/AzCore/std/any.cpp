#include <AzCore/std/any.h>
#include <AzCore\EBus\EBus.h>
#include <AzCore\Component\ComponentApplicationBus.h>
#include <AzCore\Serialization\SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

namespace AZStd { 
    AZStd::unordered_map<AZ::Uuid, any::CompareFnT> any::compareFunctions = AZStd::unordered_map<AZ::Uuid, any::CompareFnT>();

    bool any::make_compare_fn(AZ::Uuid type, any::CompareFnT& fn) { 
        AZ::SerializeContext* context;

        EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
        if (!context) {
            AZ_Error("any", false, "Can't find valid serialize context. any data cannot be compared without it.");
            return false;
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(type);
        if (!classData) { 
            AZ_Error("any", false, "Can't find class data for typeID %s", type.ToString<AZStd::string>().c_str()); 
            return false;
        }

        if (classData->m_serializer) {
            fn = [classData](const void* x, const void* y) {
                return classData->m_serializer->CompareValueData(x,y);
            };
        } else {
            fn = [type, context, classData](const void* x, const void* y) {
                AZStd::vector<AZ::u8> myData;
                AZ::IO::ByteContainerStream<decltype(myData)> myDataStream(&myData);

                AZ::Utils::SaveObjectToStream(myDataStream, AZ::ObjectStream::ST_BINARY, x, type, context, classData);

                AZStd::vector<AZ::u8> otherData;
                AZ::IO::ByteContainerStream<decltype(otherData)> otherDataStream(&otherData);

                AZ::Utils::SaveObjectToStream(otherDataStream, AZ::ObjectStream::ST_BINARY, y, type, context, classData);

                return (myData.size() == otherData.size()) && (memcmp(myData.data(), otherData.data(), myData.size()) == 0);
            };
        }
        return true;
    }

    bool any::find_compare_fn(AZ::Uuid type, any::CompareFnT& fn) { 
        auto it = compareFunctions.find(type);
        if (it != compareFunctions.end()) { 
            fn = it->second;
            return true;
        } else { 
            CompareFnT newFn;
            if (make_compare_fn(type, newFn)) { 
                compareFunctions[type] = newFn;
                fn = newFn;
                return true;
            } 
        }
        return false;
    }

    bool any::operator==(const any& o) const { 
        if (empty() && o.empty()) return true;
        if (type() != o.type()) return false;
        CompareFnT fn;
        if (find_compare_fn(type(), fn)) { 
            return fn(get_data(), o.get_data());
        } else { 
            return false;
        }
    }

}