-- TODO: get require working
-- local DisintegrateLib = require '../../../Gems/Atom/Feature/Common/Assets/Materials/Types/MaterialInputs/DisintegrateEffectLib.lua'

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
    local shadowMapWitPS = context:GetShaderByTag("Shadowmap_WithPS")
    local forwardPass = context:GetShaderByTag("ForwardPass")
    local transparentPass = context:GetShaderByTag("TransparentPass");
    local momentGenerationPass = context:GetShaderByTag("OITMomentGenerationPass");

    local isTransparent = (opacityMode == OpacityMode_Blended) or (opacityMode == OpacityMode_TintedTransparent);
    
    context:GetShaderByTag("DepthPassTransparentMin"):SetEnabled(isTransparent)
    context:GetShaderByTag("DepthPassTransparentMax"):SetEnabled(isTransparent)
    transparentPass:SetEnabled(isTransparent)
    momentGenerationPass:SetEnabled(isTransparent)

    if (isTransparent) then
       -- Disable all opaque passes
       local opaquePasses = { depthPass, shadowMap, forwardPassEDS, depthPassWithPS, shadowMapWitPS, forwardPass };
       for _, pass in next, opaquePasses do
          pass:SetEnabled(false)
       end

    else
       if parallaxEnabled and parallaxPdoEnabled then
          depthPass:SetEnabled(false)
          shadowMap:SetEnabled(false)
          forwardPassEDS:SetEnabled(false)
          
          depthPassWithPS:SetEnabled(true)
          shadowMapWitPS:SetEnabled(true)
          forwardPass:SetEnabled(true)
       else

          local isDisintegrateEffect = DisintegrateLib.IsDisintegrateEffectEnabled(context)

          depthPass:SetEnabled(opacityMode == OpacityMode_Opaque and not isDisintegrateEffect)
          shadowMap:SetEnabled(opacityMode == OpacityMode_Opaque and not isDisintegrateEffect)
          forwardPassEDS:SetEnabled(
             ((opacityMode == OpacityMode_Opaque) or (opacityMode == OpacityMode_Blended) or (opacityMode == OpacityMode_TintedTransparent))
             and not isDisintegrateEffect
          )

          depthPassWithPS:SetEnabled(opacityMode == OpacityMode_Cutout or isDisintegrateEffect)
          shadowMapWitPS:SetEnabled(opacityMode == OpacityMode_Cutout or isDisintegrateEffect)
          forwardPass:SetEnabled(opacityMode == OpacityMode_Cutout or isDisintegrateEffect)
       end
    end
end
