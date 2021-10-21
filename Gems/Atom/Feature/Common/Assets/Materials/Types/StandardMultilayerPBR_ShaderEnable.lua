--------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
   local deps = {"parallax.enable", "parallax.pdo"}

   for _, prop in ipairs(DisintegrateLib.GetMaterialPropertyDependencies_DisintegrateEffectIsEnabled()) do
      table.insert(deps, prop)
   end

   return deps
end

function Process(context)
    local parallaxEnabled = context:GetMaterialPropertyValue_bool("parallax.enable")
    local parallaxPdoEnabled = context:GetMaterialPropertyValue_bool("parallax.pdo")
    local isDisintegrateEffect = DisintegrateLib.IsDisintegrateEffectEnabled(context)
    
    local depthPass = context:GetShaderByTag("DepthPass")
    local shadowMap = context:GetShaderByTag("Shadowmap")
    local forwardPassEDS = context:GetShaderByTag("ForwardPass_EDS")
    local depthPassWithPS = context:GetShaderByTag("DepthPass_WithPS")
    local shadowMapWithPS = context:GetShaderByTag("Shadowmap_WithPS")
    local forwardPass = context:GetShaderByTag("ForwardPass")
    
    local shadingAffectsDepth = (parallaxEnabled and parallaxPdoEnabled) or isDisintegrateEffect;
    
    depthPass:SetEnabled(not shadingAffectsDepth)
    shadowMap:SetEnabled(not shadingAffectsDepth)
    forwardPassEDS:SetEnabled(not shadingAffectsDepth)
        
    depthPassWithPS:SetEnabled(shadingAffectsDepth)
    shadowMapWithPS:SetEnabled(shadingAffectsDepth)
    forwardPass:SetEnabled(shadingAffectsDepth)
end
