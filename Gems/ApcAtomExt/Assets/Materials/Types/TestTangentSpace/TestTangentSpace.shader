{
    "Source": "TestTangentSpace.azsl",

    "DepthStencilState": {
        "Depth": {
            "Enable": true,
            "CompareFunc": "GreaterEqual"
        }
    },

    // Using auxgeom draw list to avoid tonemapping
    "DrawList": "auxgeom",

    "ProgramSettings": {
        "EntryPoints": [
            {
                "name": "CommonVS",
                "type": "Vertex"
            },
            {
                "name": "MainPS",
                "type": "Fragment"
            }
        ]
    }
}
