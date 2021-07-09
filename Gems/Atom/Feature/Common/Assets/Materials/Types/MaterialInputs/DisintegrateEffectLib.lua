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


return DisintegrateLib
