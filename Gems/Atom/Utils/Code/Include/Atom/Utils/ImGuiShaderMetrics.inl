/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Shader/Metrics/ShaderMetricsSystemInterface.h>

namespace AZ
{
    namespace Render
    {
        inline void ImGuiShaderMetrics::Draw(bool& draw, const RPI::ShaderVariantMetrics& metrics)
        {
            ImVec2 windowSize(600, 500.f);
            ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
            if (ImGui::Begin("Shader Metrics", &draw, ImGuiWindowFlags_None))
            {
                auto shaderMetricsSystem = AZ::RPI::ShaderMetricsSystemInterface::Get();

                if (ImGui::Button("Reset"))
                {
                    shaderMetricsSystem->Reset();
                }

                if (ImGui::Button("Write Log"))
                {
                    shaderMetricsSystem->WriteLog();
                }

                bool enableMetrics = shaderMetricsSystem->IsEnabled();
                if (ImGui::Checkbox("Enable Metrics", &enableMetrics))
                {
                    shaderMetricsSystem->SetEnabled(enableMetrics);
                }

                bool enableAutoAddMissingVariants = shaderMetricsSystem->IsAutomaticallyAddingMissingShaderVariants();
                if (ImGui::Checkbox("Enable Auto-Add Missing Variants", &enableAutoAddMissingVariants))
                {
                    shaderMetricsSystem->SetAutomaticallyAddingMissingShaderVariants(enableAutoAddMissingVariants);
                }

                ImGui::Separator();

                // Set column settings.
                ImGui::Columns(4, "view", false);
                ImGui::SetColumnWidth(0, 100.0f);
                ImGui::SetColumnWidth(1, 300.0f);
                ImGui::SetColumnWidth(2, 100.0f);
                ImGui::SetColumnWidth(3, 100.0f);

                ImGui::Text("Requests");
                ImGui::NextColumn();

                ImGui::Text("Shader");
                ImGui::NextColumn();

                ImGui::Text("Variant");
                ImGui::NextColumn();

                ImGui::Text("Branches");
                ImGui::NextColumn();

                struct RequestResultRowEntry
                {
                    RPI::ShaderVariantRequest request;
                    RPI::ShaderVariantResult result;
                    size_t requestCount = 0;
                };
                AZStd::vector<RequestResultRowEntry> flattenedRequests;

                for (const auto& request : metrics.m_requests)
                {
                    flattenedRequests.push_back();
                    RequestResultRowEntry& entry = flattenedRequests.back();
                    entry.request = request.first;

                    // Note: assumes each request maps to a unique result, which isn't the case if the shadervariantlist
                    // changed, or a shader variant finished compiling, somewhere in the middle of gathering metrics.
                    if (!request.second.empty())
                    {
                        entry.result = request.second.begin()->first;
                    }

                    for (const auto& result : request.second)
                    {
                        entry.requestCount += result.second;
                    }
                }

                auto sortFunction = [](const RequestResultRowEntry& left, const RequestResultRowEntry& right)
                {
                    return left.requestCount < right.requestCount;
                };

                AZStd::sort(flattenedRequests.begin(), flattenedRequests.end(), sortFunction);

                for (const auto& request : flattenedRequests)
                {
                    ImGui::Text("%d", request.requestCount);
                    ImGui::NextColumn();

                    ImGui::Text("%s", request.request.m_shaderName.GetCStr());
                    ImGui::NextColumn();

                    ImGui::Text("%d", request.result.m_shaderVariantStableId.GetIndex());
                    ImGui::NextColumn();

                    if (request.result.m_dynamicOptionCount == 0)
                    {
                        ImGui::Text("%d", request.result.m_dynamicOptionCount);
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%d", request.result.m_dynamicOptionCount);
                    }
                    ImGui::NextColumn();
                }
            }
            ImGui::End();
        }
    } // namespace Render
} // namespace AZ
