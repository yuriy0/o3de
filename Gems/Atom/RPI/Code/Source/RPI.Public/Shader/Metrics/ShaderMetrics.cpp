/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Public/Shader/Metrics/ShaderMetrics.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

namespace AZ
{
    namespace RPI
    {
        static ShaderVariantMetrics s_metrics;


        void PrettyShaderVariantId::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PrettyShaderVariantId>()
                    ->Version(1)
                    ->Field("Options", &PrettyShaderVariantId::m_options)
                    ;
            }
        }

        void ShaderVariantResult::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantResult>()
                    ->Version(1)
                    ->Field("ShaderVariantStableId", &ShaderVariantResult::m_shaderVariantStableId)
                    ->Field("DynamicOptionCount", &ShaderVariantResult::m_dynamicOptionCount)
                    ;
            }
        }

        bool ShaderVariantResult::operator==(const ShaderVariantResult& other) const
        {
            return m_shaderVariantStableId == other.m_shaderVariantStableId;
        }

        bool ShaderVariantResult::operator<(const ShaderVariantResult& other) const
        {
            return m_shaderVariantStableId < other.m_shaderVariantStableId;
        }

        void ShaderVariantRequest::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantRequest>()
                    ->Version(1)
                    ->Field("ShaderId", &ShaderVariantRequest::m_shaderId)
                    ->Field("ShaderName", &ShaderVariantRequest::m_shaderName)
                    ->Field("ShaderVariantId", &ShaderVariantRequest::m_shaderVariantId)
                    ->Field("PrettyShaderVariantId", &ShaderVariantRequest::m_prettyShaderVariantId)
                    ;
            }
        }

        bool ShaderVariantRequest::operator==(const ShaderVariantRequest& other) const
        {
            return m_shaderId == other.m_shaderId && m_shaderVariantId == other.m_shaderVariantId;
        }

        bool ShaderVariantRequest::operator<(const ShaderVariantRequest& other) const
        {
            if (m_shaderId != other.m_shaderId)
            {
                return m_shaderId < other.m_shaderId;
            }
            else
            {
                return m_shaderVariantId < other.m_shaderVariantId;
            }
        }

        void ShaderVariantMetrics::Reflect(AZ::ReflectContext* context)
        {
            PrettyShaderVariantId::Reflect(context);
            ShaderVariantRequest::Reflect(context);
            ShaderVariantResult::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantMetrics>()
                    ->Version(1)
                    ->Field("m_requests", &ShaderVariantMetrics::m_requests)
                    ;
            }
        }

    } // namespace RPI
} // namespace AZ
