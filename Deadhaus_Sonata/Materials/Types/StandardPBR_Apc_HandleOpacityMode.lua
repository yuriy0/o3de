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
    return {"opacity.mode", "opacity.alphaSource", "opacity.textureMap"}
end
 
OpacityMode_Opaque = 0
OpacityMode_Cutout = 1
OpacityMode_Blended = 2
OpacityMode_TintedTransparent = 3

AlphaSource_Packed = 0
AlphaSource_Split = 1
AlphaSource_None = 2

ForwardPassIndex = 1

function ConfigureAlphaBlending(shaderItem) 
    shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
    shaderItem:GetRenderStatesOverride():SetDepthWriteMask(DepthWriteMask_Zero)
    shaderItem:GetRenderStatesOverride():SetBlendEnabled(0, true)
    shaderItem:GetRenderStatesOverride():SetBlendSource(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendDest(0, BlendFactor_AlphaSourceInverse)
    shaderItem:GetRenderStatesOverride():SetBlendOp(0, BlendOp_Add)
end

function ConfigureDualSourceBlending(shaderItem)
    -- This blend multiplies the dest against color source 1, then adds color source 0.
    shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
    shaderItem:GetRenderStatesOverride():SetDepthWriteMask(DepthWriteMask_Zero)
    shaderItem:GetRenderStatesOverride():SetBlendEnabled(0, true)
    shaderItem:GetRenderStatesOverride():SetBlendSource(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendDest(0, BlendFactor_ColorSource1)
    shaderItem:GetRenderStatesOverride():SetBlendOp(0, BlendOp_Add)
end

function ResetAlphaBlending(shaderItem)
    shaderItem:GetRenderStatesOverride():ClearDepthEnabled()
    shaderItem:GetRenderStatesOverride():ClearDepthWriteMask()
    shaderItem:GetRenderStatesOverride():ClearBlendEnabled(0)
    shaderItem:GetRenderStatesOverride():ClearBlendSource(0)
    shaderItem:GetRenderStatesOverride():ClearBlendDest(0)
    shaderItem:GetRenderStatesOverride():ClearBlendOp(0)
end

function Process(context)
   -- NB: runtime processing for opactiy mode is handled by StandardPBR_Apc_ShaderEnable.lua, StandardPBR_Apc_SetTransparentRefractiveBlendStates.lua


    local opacityMode = context:GetMaterialPropertyValue_enum("opacity.mode") 

    local EnableAlphaToCoverage = function(shader)
       local sh = shader:GetRenderStatesOverride();

       sh:SetAlphaToCoverageEnabled(true)

       -- sh:SetIndependentBlendEnabled(true)
       -- sh:SetBlendEnabled(0, true)

       -- sh:SetBlendSource(0, BlendFactor_AlphaSource)
       -- sh:SetBlendOp(0, BlendOp_Add)
       -- sh:SetBlendDest(0, BlendFactor_AlphaSourceInverse)

       -- sh:SetBlendAlphaSource(0, BlendFactor_One)
       -- sh:SetBlendAlphaOp(0, BlendOp_Add)
       -- sh:SetBlendAlphaDest(0, BlendFactor_Zero)
    end

    -- Handle alpha test
    if (opacityMode == OpacityMode_Cutout) then
       EnableAlphaToCoverage(context:GetShaderByTag("ForwardPass"))
       EnableAlphaToCoverage(context:GetShaderByTag("ForwardPass_EDS"))
       EnableAlphaToCoverage(context:GetShaderByTag("DepthPass_WithPS"))
       EnableAlphaToCoverage(context:GetShaderByTag("Shadowmap_WithPS"))
    end
end

function ProcessEditor(context)
    local opacityMode = context:GetMaterialPropertyValue_enum("opacity.mode")
    
    local mainVisibility
    if(opacityMode == OpacityMode_Opaque) then
        mainVisibility = MaterialPropertyVisibility_Hidden
    else
        mainVisibility = MaterialPropertyVisibility_Enabled
    end
    
    -- Handle properties only available for non-opaque materials
    local nonOpaqueMaterialProperties = {
       "opacity.alphaSource", "opacity.textureMap", "opacity.textureMapUv", "opacity.factor"
    }
    for _, prop in next, nonOpaqueMaterialProperties do
       context:SetMaterialPropertyVisibility(prop, mainVisibility) 
    end

    -- Handle properties specific to blended transparency mode
    local blendedOnlyMaterialProperties = {
       "opacity.transmittanceTint", "opacity.collimationFactor",
       "opacity.thicknessToGapSizeRatio", "opacity.oitBlendFunction", "opacity.oitClip"
    };
    
    local blendedOnlyVisibility = (opacityMode == OpacityMode_Blended or opacityMode == OpacityMode_TintedTransparent);
    blendedOnlyVisibility = (blendedOnlyVisibility and MaterialPropertyVisibility_Enabled) or MaterialPropertyVisibility_Hidden

    for _, prop in next, blendedOnlyMaterialProperties do
       context:SetMaterialPropertyVisibility(prop, blendedOnlyVisibility)
    end

    -- Double sided should always be available, and probably should not be in the opacity section
    context:SetMaterialPropertyVisibility("opacity.doubleSided", MaterialPropertyVisibility_Enabled)

    -- If transparency is enabled, handle show/hide of opacity texture options
    if(mainVisibility == MaterialPropertyVisibility_Enabled) then
        local alphaSource = context:GetMaterialPropertyValue_enum("opacity.alphaSource")

        if(alphaSource ~= AlphaSource_Split) then
            context:SetMaterialPropertyVisibility("opacity.textureMap", MaterialPropertyVisibility_Hidden)
            context:SetMaterialPropertyVisibility("opacity.textureMapUv", MaterialPropertyVisibility_Hidden)
        else
            local textureMap = context:GetMaterialPropertyValue_Image("opacity.textureMap")

            if(nil == textureMap) then
                context:SetMaterialPropertyVisibility("opacity.textureMapUv", MaterialPropertyVisibility_Disabled)
                context:SetMaterialPropertyVisibility("opacity.factor", MaterialPropertyVisibility_Disabled)
            end
        end
    end
end
