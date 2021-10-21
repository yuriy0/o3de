{
    "Source" : "TransparentRefractiveDepthWrite",

    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : true,
            "CompareFunc" : "GreaterEqual"
        },
        "Stencil" :
        {
            "Enable" : true,
            "ReadMask" : "0x00",
            "WriteMask" : "0xFF",
            "FrontFace" :
            {
                "Func" : "Always",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Replace"
            }
        }
    },

    "BlendState":
    {
        "Enable": false
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MainVS",
          "type": "Vertex"
        },
        {
          "name": "MainPS",
          "type": "Fragment"
        }
      ]
    }
}
