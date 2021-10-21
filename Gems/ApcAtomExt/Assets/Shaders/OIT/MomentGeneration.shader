{
    "Source" : "./MomentGeneration.azsl",

    "RasterState": { "CullMode": "None" },

    "DepthStencilState" : {
        "Depth" : {
            "Enable" : true,
            "CompareFunc" : "GreaterEqual",
            "WriteMask" : "Zero"
        },
        "Stencil" :
        {
            "Enable" : false
        }
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MomentGeneration_VS",
          "type": "Vertex"
        },
        {
          "name": "MomentGeneration_PS",
          "type": "Fragment"
        }
      ]
    },

    "CompilerHints" : {
    },

    "DrawList" : "momentOitGeneration"
}
