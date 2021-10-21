/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>

#include "Atom/RPI.Reflect/Shader/ShaderVariantKey.h"

namespace AZ
{
    namespace RPI
    {
        class ShaderAsset;

        struct PrettyShaderVariantId
        {
            AZ_TYPE_INFO(PrettyShaderVariantId, "{BE1F6C6F-087C-4EA8-BF54-DDA5E7C5213A}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::unordered_map<AZStd::string, AZStd::string> m_options;
        };

        struct ShaderVariantRequest
        {
            AZ_TYPE_INFO(ShaderVariantRequest, "{013FF2BB-6510-47CD-88C1-4CA855968E29}");
            static void Reflect(AZ::ReflectContext* context);

            bool operator==(const ShaderVariantRequest& other) const;
            bool operator<(const ShaderVariantRequest& other) const;

            //! The ID of the shader.
            AZ::Data::AssetId m_shaderId;

            //! The name of the shader.
            Name m_shaderName;

            //! The ID of the shader variant.
            ShaderVariantId m_shaderVariantId;

            //! Human-readable shader variant ID
            PrettyShaderVariantId m_prettyShaderVariantId;
        };

        struct ShaderVariantResult
        {
            AZ_TYPE_INFO(ShaderVariantResult, "{C35727AE-0E3C-480E-9551-0537B79B10D6}");
            static void Reflect(AZ::ReflectContext* context);

            bool operator==(const ShaderVariantResult& other) const;
            bool operator<(const ShaderVariantResult& other) const;

            //! The index of the shader variant.
            ShaderVariantStableId m_shaderVariantStableId;

            //! The number of dynamic options in the variant.
            uint32_t m_dynamicOptionCount;
        };

        struct ShaderVariantMetrics
        {
            AZ_TYPE_INFO(ShaderVariantMetrics, "{F368F633-449D-4B31-83E8-A44C945C9B78}");
            AZ_CLASS_ALLOCATOR(ShaderVariantMetrics, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            AZStd::map<ShaderVariantRequest, AZStd::map<ShaderVariantResult, size_t>> m_requests;
        };

        //////////////////////////////////////////////////////////////////////////
    } // namespace RPI

} // namespace AZ
