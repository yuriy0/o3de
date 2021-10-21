{ 
    "Source" : "ShaderLib/Apc/Features/Pbr/TransparentRefractivePassSrg.azsli",

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : false 
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
          "name": "MainVS",
          "type": "Vertex"
        },
        {
          "name": "MainPS",
          "type": "Fragment"
        }
      ]
    },
    
    "Supervariants":
    [
        {
            "Name": "",
            "PlusArguments": "",
            "MinusArguments": "--strip-unused-srgs"
        }
    ]
}
