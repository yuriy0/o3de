-- local DisintegrateLib = require './DisintegrateEffectLib.lua'

-- TODO: get require working
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
    return {
        "disintegrate.mask",
        "disintegrate.tint",
        "disintegrate.thickness",
        "disintegrate.percentage",
    }
end

function GetShaderOptionDependencies()
    return {
        "o_disintegrate_effect_enabled",
    };
end

function Process(context)
    local isFeatureEnabled = DisintegrateLib.IsDisintegrateEffectEnabled(context)

    -- Print("Disintegrate feature enabled: " .. tostring(isFeatureEnabled))

    context:SetShaderOptionValue_bool("o_disintegrate_effect_enabled", isFeatureEnabled)
end

function ProcessEditor(context)
    local isFeatureEnabled = DisintegrateLib.IsDisintegrateEffectEnabled(context)

    local properties = {
        "disintegrate.tint",
        "disintegrate.thickness",
        "disintegrate.percentage"
    };

    local mainVisibility = (isFeatureEnabled and MaterialPropertyVisibility_Enabled) or MaterialPropertyVisibility_Disabled

    for _, property in next, properties do
       context:SetMaterialPropertyVisibility(property, mainVisibility)
    end
end
