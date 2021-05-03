/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/MathMarshal.h>
#include <GridMate/Serialize/CompressionMarshal.h>

#include <AzCore/std/containers/variant.h>


namespace EMotionFX
{
    namespace Integration
    {
        namespace Network
        {
            /* Wrapper around AZStd::string for optimized marshalling of AnimGraph String parameter values
             */
            class AnimGraphString
            {
            public:
                AnimGraphString() = default;
                AnimGraphString(AZStd::string_view s) : str(s) {}

                AZStd::string str;

                inline bool operator==(const AnimGraphString& other) const { return str == other.str; }
                inline bool operator!=(const AnimGraphString& other) const { return !(*this == other); }
            };
        }
    }
}

namespace GridMate
{
    /**
    Marshaller for monostate. It stores no data, so do nothing for read and write.
    */
    template<typename X>
    class Marshaler<EMotionFX::Integration::Network::AnimGraphString, X>
    {
    public:
        using Type = EMotionFX::Integration::Network::AnimGraphString;
        AZ_TYPE_INFO_LEGACY(Marshaler, "{52D236CB-D528-4DB5-BC0F-EAE10395A1AC}", Type, X);

        // Writes a fixed-size string - increase this if necessary!
        static const AZStd::size_t MarshalSize = 64;

        inline void Marshal(WriteBuffer& wb, const Type& x) const {
            const auto& s = x.str;
            AZ_Error("EMotionFX", s.size() <= MarshalSize,
                     "AnimGraph string parameter '%s' is too long to send over network! "
                     "Maximum support size=%llu, actual size=%llu. "
                     "Note: You may increase this size by changing AnimGraphNetSyncTypes.h source file.",
                     s.c_str(), MarshalSize, s.size()
            );

            const PackedSize sizeBefore = wb.GetExactSize();

            const size_t stringDataSz = AZ::GetMin(s.size(), MarshalSize);
            wb.WriteRaw(s.data(), stringDataSz);
            if (stringDataSz < MarshalSize) {
                const size_t paddingSz = MarshalSize - stringDataSz;
                for (auto i = 0; i < paddingSz; ++i) { wb.Write<AZ::u8>(0); }
            }

            const PackedSize sizeAfter = wb.GetExactSize();
            const PackedSize sizeDelta = sizeAfter - sizeBefore;
            AZ_Assert(sizeDelta == PackedSize{MarshalSize},
                      "AnimGraphString::Marshal - internal error, expected to write %llu bytes but wrote %llu + %llu bits",
                      MarshalSize, sizeDelta.GetBytes(), sizeDelta.GetAdditionalBits()
            );
        }
        inline void Unmarshal(Type& x, ReadBuffer& rb) const {
            const PackedSize sizeBefore = rb.Read();

            char rawData[MarshalSize+1]{};
            rawData[MarshalSize] = 0;
            rb.ReadRaw(rawData, MarshalSize);
            x.str = rawData;

            const PackedSize sizeAfter = rb.Read();
            const PackedSize sizeDelta = sizeAfter - sizeBefore;
            AZ_Assert(sizeDelta == PackedSize{MarshalSize},
                        "AnimGraphString::Unmarshal - internal error, expected to read %llu bytes but read %llu + %llu bits",
                        MarshalSize, sizeDelta.GetBytes(), sizeDelta.GetAdditionalBits()
            );
        }
    };

    /**
    Marshaller specialization for AnimGraphString
    */
    template<typename X>
    class Marshaler<AZStd::monostate, X>
    {
    public:
        using Type = AZStd::monostate;
        AZ_TYPE_INFO_LEGACY(Marshaler, "{2306A3E0-D534-4B61-BED3-1DFEF453B8B1}", AZStd::monostate, X);
        static const AZStd::size_t MarshalSize = 0;

        void Marshal(WriteBuffer&, const Type&) const {}
        void Unmarshal(Type&, ReadBuffer&) const {}
    };
}

namespace EMotionFX
{
    namespace Integration
    {
        namespace Network
        {
            using AnimParameterImpl = AZStd::variant<AZStd::monostate, float, bool, AZ::Vector2, AZ::Vector3, AZ::Quaternion, AnimGraphString>;

            /**
             * \brief A general storage for an anim graph parameter.
             */
            class AnimParameter
                : public AnimParameterImpl
            {
            public:
                enum class Type : AZ::u8
                {
                    Unsupported,
                    Float,
                    Bool,
                    Vector2,
                    Vector3,
                    Quaternion,
                    String,
                    COUNT
                };

                AnimParameter() : AnimParameterImpl(AZStd::monostate{}) {}

                AnimParameter(Type t)
                    : AnimParameterImpl(GetValueForIndex(t))
                {}

                template<Type Ix, class... Args>
                void Assign(Args&&... args)
                {
                    static_cast<AnimParameterImpl*>(this)->emplace<(size_t)Ix>(AZStd::forward<Args>(args)...);
                }

                void Set(Type t)
                {
                    *static_cast<AnimParameterImpl*>(this) = GetValueForIndex(t);
                }

                AnimParameter(const AnimParameter& other) = default;
                AnimParameter& operator=(const AnimParameter& other) = default;

                inline friend bool operator==(const AnimParameter& lhs, const AnimParameter& rhs)
                {
                    return static_cast<const AnimParameterImpl&>(lhs) == static_cast<const AnimParameterImpl&>(rhs);
                }

            private:
                inline static AnimParameterImpl GetValueForIndex(Type t)
                {
                    static const AZStd::array<AnimParameterImpl, size_t(Type::COUNT)> values =
                    {
                        AnimParameterImpl(AZStd::in_place_index_t<size_t(Type::Unsupported)>{}),
                        AnimParameterImpl(AZStd::in_place_index_t<size_t(Type::Float)>{}),
                        AnimParameterImpl(AZStd::in_place_index_t<size_t(Type::Bool)>{}),
                        AnimParameterImpl(AZStd::in_place_index_t<size_t(Type::Vector2)>{}),
                        AnimParameterImpl(AZStd::in_place_index_t<size_t(Type::Vector3)>{}),
                        AnimParameterImpl(AZStd::in_place_index_t<size_t(Type::Quaternion)>{}),
                        AnimParameterImpl(AZStd::in_place_index_t<size_t(Type::String)>{}),
                    };
                    return values[(size_t)t];
                };
            };


            /**
             * \brief Custom GridMate throttler. See GridMate:: @BasicThrottle
             */
            class AnimParameterThrottler
            {
            public:
                bool WithinThreshold(const AnimParameter& newValue) const
                {
                    return m_baseline == newValue;
                }

                void UpdateBaseline(const AnimParameter& baseline)
                {
                    m_baseline = baseline;
                }

            private:
                AnimParameter m_baseline;
            };

            /**
             * \brief A custom GridMate marshaler.
             * 1 byte is spend on the type. And a variable number of bytes afterwards for the value.
             */
            class AnimParameterMarshaler
            {
            public:
                void Marshal(GridMate::WriteBuffer& wb, const AnimParameter& parameter)
                {
                    wb.Write(AZ::u8(parameter.index()));
                    AZStd::visit([&wb](const auto& val) { wb.Write(val); }, parameter);
                }

                void Unmarshal(AnimParameter& parameter, GridMate::ReadBuffer& rb)
                {
                    AZ::u8 type;
                    rb.Read(type);
                    parameter.Set(AnimParameter::Type(type));
                    AZStd::visit([&rb](auto& val) { rb.Read(val); }, parameter);
                }
            };
            
            /**
             * \brief Custom marshaler for Animation node index that is used by Activate Nodes list
             */
            struct NodeIndexContainerMarshaler
            {
                void Marshal(GridMate::WriteBuffer& wb, const NodeIndexContainer& source) const
                {
                    GridMate::VlqU64Marshaler m64;
                    GridMate::VlqU32Marshaler m32;

                    m64.Marshal(wb, source.size()); // 1 byte most of the time (if the size is less than 127)
                    for (AZ::u32 item : source)
                    {
                        m32.Marshal(wb, item); // 1 byte most of the time (if the value is less than 127)
                    }
                }

                void Unmarshal(NodeIndexContainer& target, GridMate::ReadBuffer& rb) const
                {
                    target.clear();
                    GridMate::VlqU64Marshaler m64;
                    GridMate::VlqU32Marshaler m32;

                    AZ::u64 arraySize;
                    m64.Unmarshal(arraySize, rb);
                    target.resize(arraySize);

                    for (AZ::u64 i = 0; i < arraySize; ++i)
                    {
                        m32.Unmarshal(target[i], rb);
                    }
                }
            };

            /**
             * \brief Custom marshaler for Animation motion node information that is used by motion node playtime list
             */
            struct MotionNodePlaytimeContainerMarshaler
            {
                void Marshal(GridMate::WriteBuffer& wb, const MotionNodePlaytimeContainer& source) const
                {
                    GridMate::VlqU64Marshaler m64;
                    GridMate::VlqU32Marshaler m32;

                    m64.Marshal(wb, source.size());
                    for (const AZStd::pair<AZ::u32, float>& item : source)
                    {
                        m32.Marshal(wb, item.first);    // average of 1 byte
                        wb.Write(item.second);          // 4 bytes
                    }
                }

                void Unmarshal(MotionNodePlaytimeContainer& target, GridMate::ReadBuffer& rb) const
                {
                    target.clear();
                    GridMate::VlqU64Marshaler m64;
                    GridMate::VlqU32Marshaler m32;

                    AZ::u64 arraySize;
                    m64.Unmarshal(arraySize, rb);
                    target.resize(arraySize);

                    for (AZ::u64 i = 0; i < arraySize; ++i)
                    {
                        m32.Unmarshal(target[i].first, rb);
                        rb.Read(target[i].second);
                    }
                }
            };
        }
    } // namespace Integration
} // namespace EMotionFXAnimation
