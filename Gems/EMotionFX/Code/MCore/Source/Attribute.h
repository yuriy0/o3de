/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "StandardHeaders.h"
#include "MemoryManager.h"
#include "Endian.h"
#include "Stream.h"
#include <AzCore/std/string/string.h>
#include <AzCore/std/any_ext.h>

namespace EMotionFX
{
    namespace Network
    {
        class AnimGraphSnapshotChunkSerializer;
    }
}

namespace MCore
{
    // forward declarations
    class AttributeSettings;

    // the attribute interface types
    enum : AZ::u32
    {
        ATTRIBUTE_INTERFACETYPE_FLOATSPINNER    = 0,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_FLOATSLIDER     = 1,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_INTSPINNER      = 2,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_INTSLIDER       = 3,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_COMBOBOX        = 4,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_CHECKBOX        = 5,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_VECTOR2         = 6,        // MCore::AttributeVector2
        ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO    = 7,        // MCore::AttributeVector3
        ATTRIBUTE_INTERFACETYPE_VECTOR4         = 8,        // MCore::AttributeVector4
        ATTRIBUTE_INTERFACETYPE_COLOR           = 10,       // MCore::AttributeVector4
        ATTRIBUTE_INTERFACETYPE_STRING          = 11,       // MCore::AttributeString
        ATTRIBUTE_INTERFACETYPE_TAG             = 26,       // MCore::AttributeBool
        ATTRIBUTE_INTERFACETYPE_VECTOR3         = 113212,   // MCore::AttributeVector3
        ATTRIBUTE_INTERFACETYPE_PROPERTYSET     = 113213,   // MCore::AttributeSet
        ATTRIBUTE_INTERFACETYPE_DEFAULT         = 0xFFFFFFFF// use the default attribute type that the specific attribute class defines as default
    };

    class MCORE_API Attribute
    {
        friend class AttributeFactory;
    public:
        virtual ~Attribute();

        virtual Attribute* Clone() const = 0;
        virtual const char* GetTypeString() const = 0;
        MCORE_INLINE AZ::u32 GetType() const                                         { return m_typeId; }
        virtual bool InitFromString(const AZStd::string& valueString) = 0;
        virtual bool ConvertToString(AZStd::string& outString) const = 0;
        virtual bool InitFrom(const Attribute* other) = 0;
        virtual size_t GetClassSize() const = 0;
        virtual AZ::u32 GetDefaultInterfaceType() const = 0;
        virtual bool FromAny(const AZStd::any&) = 0;
        virtual bool ToAny(AZStd::any&) = 0;

        Attribute& operator=(const Attribute& other);

        virtual void NetworkSerialize(EMotionFX::Network::AnimGraphSnapshotChunkSerializer&) {};

    protected:
        AZ::u32      m_typeId;    /**< The unique type ID of the attribute class. */

        Attribute(AZ::u32 typeID);
    };

    namespace Attribute_impl {
        using AZStd::any_generic_cast;
    }

    /**
    * A version of Attribute with template arguments which defines defaults for To/FromAny
    */
    template<class Type, class DerivedType>
    class Attribute_tpl
        : public Attribute { 
    public:
        template<class... Args>
        Attribute_tpl<Type, DerivedType>(Args&&... args)
            : Attribute(AZStd::forward<Args>(args)...)
        {}

#pragma warning(push)
#pragma warning(disable : 4804) // '<': unsafe use of type 'bool' in operation
        virtual bool FromAny(const AZStd::any& anyVal) override {
            Type realVal;
            if (Attribute_impl::any_generic_cast(anyVal, realVal)) {
                static_cast<DerivedType*>(this)->SetValue(realVal);
                return true;
            } else {
                return false;
            }
        };
#pragma warning(pop)

        virtual bool ToAny(AZStd::any& anyVal) override {
            anyVal = AZStd::any(AZStd::in_place_type_t<Type>{}, static_cast<DerivedType*>(this)->GetValue());
            return true;
        };
    };
} // namespace MCore
