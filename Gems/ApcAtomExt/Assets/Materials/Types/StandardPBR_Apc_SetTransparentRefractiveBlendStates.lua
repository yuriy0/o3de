function GetMaterialPropertyDependencies()
    return {"opacity.mode", "opacity.alphaSource", "opacity.textureMap"}
end

function SetBlendStateBasic(sh)
   sh:SetDepthEnabled(true)
   sh:SetDepthWriteMask(DepthWriteMask_Zero)
   sh:SetIndependentBlendEnabled(true)
end

function SetSumColorBlendStates(sh, rtIx)
   sh:SetBlendSource(rtIx, BlendFactor_One)
   sh:SetBlendDest(rtIx, BlendFactor_One)
   sh:SetBlendOp(rtIx, BlendOp_Add)
end

function SetProductColorBlendStates(sh, rtIx)
   sh:SetBlendSource(rtIx, BlendFactor_Zero)
   sh:SetBlendDest(rtIx, BlendFactor_ColorSourceInverse)
   sh:SetBlendOp(rtIx, BlendOp_Add)
end

function SetSumAlphaBlendStates(sh, rtIx)
   sh:SetBlendAlphaSource(rtIx, BlendFactor_One)
   sh:SetBlendAlphaDest(rtIx, BlendFactor_One)
   sh:SetBlendAlphaOp(rtIx, BlendOp_Add)
end

function SetTranparentPassRenderStates(shaderItem)
    local shaderItemRenderStates = shaderItem:GetRenderStatesOverride();

    -- shaderItem:SetDrawListTagOverride("transparentrefractive")

    SetBlendStateBasic(shaderItemRenderStates)

    -- Sum color, sum alpha
    shaderItemRenderStates:SetBlendEnabled(0, true)
    SetSumColorBlendStates(shaderItemRenderStates, 0)
    SetSumAlphaBlendStates(shaderItemRenderStates, 0)

    -- Product color, sum alpha
    shaderItemRenderStates:SetBlendEnabled(1, true)
    SetProductColorBlendStates(shaderItemRenderStates, 1)
    SetSumAlphaBlendStates(shaderItemRenderStates, 1)
end


function SetMomentGenerationRenderStates(shaderItem)
   local shaderItemRenderStates = shaderItem:GetRenderStatesOverride();

    SetBlendStateBasic(shaderItemRenderStates)

    -- Sum color, sum alpha
    shaderItemRenderStates:SetBlendEnabled(0, true)
    SetSumColorBlendStates(shaderItemRenderStates, 0)
    SetSumAlphaBlendStates(shaderItemRenderStates, 0)

    -- Sum color
    shaderItemRenderStates:SetBlendEnabled(1, true)
    SetSumColorBlendStates(shaderItemRenderStates, 1)
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
   SetTranparentPassRenderStates(context:GetShaderByTag("TransparentPass"))
   SetMomentGenerationRenderStates(context:GetShaderByTag("OITMomentGenerationPass"))
   ResetAlphaBlending(context:GetShaderByTag("ForwardPass"))
   ResetAlphaBlending(context:GetShaderByTag("ForwardPass_EDS"))
end

function ProcessEditor(context)
end
