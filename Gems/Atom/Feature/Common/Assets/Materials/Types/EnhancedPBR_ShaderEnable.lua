--------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

-- TODO: get require working
-- local DisintegrateLib = require './DisintegrateEffectLib'

local DisintegrateLib = {}

DisintegrateLib.GetMaterialPropertyDependencies_DisintegrateEffectIsEnabled = function()
    return {
        "disintegrate.mask", "disintegrate.percentage"
    }
end

DisintegrateLib.IsDisintegrateEffectEnabled = function(context)
   local mask = context:GetMaterialPropertyValue_Image("disintegrate.mask")
   local percentage = context:GetMaterialPropertyValue_float("disintegrate.percentage")
   return mask ~= nil and percentage > 0
end

-- END TODO


function GetMaterialPropertyDependencies()
   local deps = {"opacity.mode", "parallax.textureMap", "parallax.useTexture", "parallax.pdo"}

   for _, prop in ipairs(DisintegrateLib.GetMaterialPropertyDependencies_DisintegrateEffectIsEnabled()) do
      table.insert(deps, prop)
   end

   return deps
end

OpacityMode_Opaque = 0
OpacityMode_Cutout = 1
OpacityMode_Blended = 2
OpacityMode_TintedTransparent = 3

function TryGetShaderByTag(context, shaderTag)
    if context:HasShaderWithTag(shaderTag) then
        return context:GetShaderByTag(shaderTag)
    else
        return nil
    end
end

function TrySetShaderEnabled(shader, enabled)
    if shader then 
        shader:SetEnabled(enabled)
    end
end

function Process(context)
    local opacityMode = context:GetMaterialPropertyValue_enum("opacity.mode")
    local displacementMap = context:GetMaterialPropertyValue_Image("parallax.textureMap")
    local useDisplacementMap = context:GetMaterialPropertyValue_bool("parallax.useTexture")
    local parallaxEnabled = displacementMap ~= nil and useDisplacementMap
    local parallaxPdoEnabled = context:GetMaterialPropertyValue_bool("parallax.pdo")
    
    local depthPass = context:GetShaderByTag("DepthPass")
    local shadowMap = context:GetShaderByTag("Shadowmap")
    local forwardPassEDS = context:GetShaderByTag("ForwardPass_EDS")

    local depthPassWithPS = context:GetShaderByTag("DepthPass_WithPS")
    local shadowMapWithPS = context:GetShaderByTag("Shadowmap_WithPS")
    local forwardPass = context:GetShaderByTag("ForwardPass")

    -- Use TryGetShaderByTag because these shaders only exist in StandardPBR but this script is also used for EnhancedPBR
    local lowEndForwardEDS = TryGetShaderByTag(context, "LowEndForward_EDS")
    local lowEndForward = TryGetShaderByTag(context, "LowEndForward")
    
    if parallaxEnabled and parallaxPdoEnabled then
        depthPass:SetEnabled(false)
        shadowMap:SetEnabled(false)
        forwardPassEDS:SetEnabled(false)
        
        depthPassWithPS:SetEnabled(true)
        shadowMapWithPS:SetEnabled(true)
        forwardPass:SetEnabled(true)

        TrySetShaderEnabled(lowEndForwardEDS, false)
        TrySetShaderEnabled(lowEndForward, true)
    else
        local isDisintegrateEffect = DisintegrateLib.IsDisintegrateEffectEnabled(context)

        depthPass:SetEnabled(opacityMode == OpacityMode_Opaque and not isDisintegrateEffect)
        shadowMap:SetEnabled(opacityMode == OpacityMode_Opaque and not isDisintegrateEffect)
        forwardPassEDS:SetEnabled(
           ((opacityMode == OpacityMode_Opaque) or (opacityMode == OpacityMode_Blended) or (opacityMode == OpacityMode_TintedTransparent))
           and not isDisintegrateEffect
        )
        
        depthPassWithPS:SetEnabled(opacityMode == OpacityMode_Cutout or isDisintegrateEffect)
        shadowMapWithPS:SetEnabled(opacityMode == OpacityMode_Cutout or isDisintegrateEffect)
        forwardPass:SetEnabled(opacityMode == OpacityMode_Cutout or isDisintegrateEffect)

        -- Only enable lowEndForwardEDS in Opaque mode, Transparent mode will be handled by forwardPassEDS. The transparent pass uses the "transparent" draw tag
        -- for both standard and low end pipelines, so this keeps both shaders from rendering to the transparent draw list.
        TrySetShaderEnabled(lowEndForwardEDS, opacityMode == OpacityMode_Opaque and not isDisintegrateEffect)
        TrySetShaderEnabled(lowEndForward, opacityMode == OpacityMode_Cutout and not isDisintegrateEffect)
    end
    
    context:GetShaderByTag("DepthPassTransparentMin"):SetEnabled((opacityMode == OpacityMode_Blended) or (opacityMode == OpacityMode_TintedTransparent))
    context:GetShaderByTag("DepthPassTransparentMax"):SetEnabled((opacityMode == OpacityMode_Blended) or (opacityMode == OpacityMode_TintedTransparent))
end
