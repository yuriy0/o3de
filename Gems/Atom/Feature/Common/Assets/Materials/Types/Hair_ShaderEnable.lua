--------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
----------------------------------------------------------------------------------------------------

function GetMaterialPropertyDependencies()
    return {"opacity.mode"}
end

OpacityMode_Opaque = 0
OpacityMode_Cutout = 1
OpacityMode_Blended = 2
OpacityMode_TintedTransparent = 3

DisplayOnlyPass_None = 0
DisplayOnlyPass_OnlyOpaque = 1
DisplayOnlyPass_OnlyTransparent = 2

function Process(context)
    local opacityMode = context:GetMaterialPropertyValue_enum("opacity.mode")
    
    local shadowMapWitPS = context:GetShaderByTag("Shadowmap_WithPS")
    local forwardPass = context:GetShaderByTag("ForwardPass")
    local transparentPass = context:GetShaderByTag("TransparentPass");
    local momentGenerationPass = context:GetShaderByTag("OITMomentGenerationPass");

    -- TODO: figure out if any transparent pass is needed
    -- local isTransparent = (opacityMode == OpacityMode_Blended) or (opacityMode == OpacityMode_TintedTransparent);
    local isTransparent = false;
    
    context:GetShaderByTag("DepthPassTransparentMin"):SetEnabled(true)
    context:GetShaderByTag("DepthPassTransparentMax"):SetEnabled(true)
    
    transparentPass:SetEnabled(isTransparent)
    momentGenerationPass:SetEnabled(isTransparent)

    shadowMapWitPS:SetEnabled(true)
    forwardPass:SetEnabled(true)
    
end
