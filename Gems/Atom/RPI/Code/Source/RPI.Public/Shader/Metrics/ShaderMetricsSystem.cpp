/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Shader/Metrics/ShaderMetricsSystem.h>
#include <Atom/RPI.Public/Shader/Metrics/ShaderMetrics.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>


#include <Atom/RPI.Edit/Shader/ShaderVariantEditorManipulationBus.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Utils/Utils.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <AzFramework/IO/LocalFileIO.h>

namespace AZ
{
    namespace RPI
    {
        AZ::IO::FixedMaxPath GetMetricsFilePath()
        {
            AZ::IO::FixedMaxPath resolvedPath;
            AZ::IO::LocalFileIO::GetInstance()->ResolvePath(resolvedPath, "@user@/ShaderMetrics.json");
            return resolvedPath;
        }

        ShaderMetricsSystemInterface* ShaderMetricsSystemInterface::Get()
        {
            return Interface<ShaderMetricsSystemInterface>::Get();
        }

        void ShaderMetricsSystem::Reflect(ReflectContext* context)
        {
            ShaderVariantMetrics::Reflect(context);
        }

        void ShaderMetricsSystem::Init()
        {
            // Register the system to the interface.
            Interface<ShaderMetricsSystemInterface>::Register(this);

            ReadLog();
        }

        void ShaderMetricsSystem::Shutdown()
        {
            WriteLog();

            // Unregister the system to the interface.
            Interface<ShaderMetricsSystemInterface>::Unregister(this);
        }

        void ShaderMetricsSystem::Reset()
        {
            m_metrics.m_requests.clear();
        }

        void ShaderMetricsSystem::ReadLog()
        {
            const AZ::IO::FixedMaxPath metricsFilePath = GetMetricsFilePath();

            if (AZ::IO::LocalFileIO::GetInstance()->Exists(metricsFilePath.c_str()))
            {
                auto loadResult = AZ::JsonSerializationUtils::LoadObjectFromFile<ShaderVariantMetrics>(m_metrics, metricsFilePath.c_str());

                if (!loadResult.IsSuccess())
                {
                    AZ_Error("ShaderMetrics", false, "Unable to read %s file", metricsFilePath.c_str());
                    return;
                }
            }
        }

        void ShaderMetricsSystem::WriteLog()
        {
            const AZ::IO::FixedMaxPath metricsFilePath = GetMetricsFilePath();

            auto saveResult = AZ::JsonSerializationUtils::SaveObjectToFile<ShaderVariantMetrics>(&m_metrics, metricsFilePath.c_str());

            if (!saveResult.IsSuccess())
            {
                AZ_Error("ShaderMetrics", false, "Unable to write %s file", metricsFilePath.c_str());
                return;
            }
        }

        bool ShaderMetricsSystem::IsEnabled() const
        {
            return m_isEnabled;
        }

        void ShaderMetricsSystem::SetEnabled(bool value)
        {
            m_isEnabled = value;
        }

        bool ShaderMetricsSystem::IsAutomaticallyAddingMissingShaderVariants() const
        {
            return m_isAddingMissingShaderVariants;
        }

        void ShaderMetricsSystem::SetAutomaticallyAddingMissingShaderVariants(bool value)
        {
            m_isAddingMissingShaderVariants = value;
        }

        const ShaderVariantMetrics& ShaderMetricsSystem::GetMetrics() const
        {
            return m_metrics;
        }

        static PrettyShaderVariantId GetPrettyShaderVariantId(const ShaderAsset* shader, const ShaderVariantId& shaderVariantId)
        {
            PrettyShaderVariantId prettyId;

            ConstPtr<ShaderOptionGroupLayout> shaderOptions = shader->GetShaderOptionGroupLayout();
            const ShaderOptionGroup shaderOptionGroup(shaderOptions, shaderVariantId);

            for (const ShaderOptionDescriptor& option : shaderOptions->GetShaderOptions())
            {
                const ShaderOptionValue optionValue = option.Get(shaderOptionGroup);
                const Name optionValueName = option.GetValueName(optionValue);
                if (!optionValueName.IsEmpty())
                {
                    prettyId.m_options[option.GetName().GetStringView()] = optionValueName.GetStringView();
                }
            }

            return prettyId;
        }

        void ShaderMetricsSystem::RequestShaderVariant(const ShaderAsset* shader, const ShaderVariantId& shaderVariantId, const ShaderVariantSearchResult& result)
        {
            if (!m_isEnabled)
            {
                return;
            }

            AZ_PROFILE_SCOPE(RPI, "ShaderMetricsSystem: RequestShaderVariant");

            ShaderVariantRequest request;
            request.m_shaderId = shader->GetId();
            request.m_shaderName = shader->GetName();
            request.m_shaderVariantId = shaderVariantId;
            request.m_prettyShaderVariantId = GetPrettyShaderVariantId(shader, shaderVariantId);

            ShaderVariantResult newResult;
            newResult.m_shaderVariantStableId = result.GetStableId();
            newResult.m_dynamicOptionCount = result.GetDynamicOptionCount();

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_metricsMutex);

                AZStd::map<ShaderVariantResult, size_t>& results = m_metrics.m_requests[request];
                auto [resultIt, didInsertNewResult] = results.emplace(newResult, 1);
                if (!didInsertNewResult)
                {
                    resultIt->second++;
                }
            }

            if (IsAutomaticallyAddingMissingShaderVariants())
            {
                // Only add the new variant if there are dynamic branches in the root variant
                if (result.GetDynamicOptionCount() > 0)
                {
                    ShaderVariantEditorManipulationBus::Broadcast(
                        &ShaderVariantEditorManipulationBus::Events::AddOptionsToShaderVariant,
                        request.m_shaderId, request.m_prettyShaderVariantId.m_options
                    );
                }
            }
        }
    }; // namespace RPI
}; // namespace AZ
