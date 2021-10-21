{ 
    "Source" : "TransparentRefractiveMergeOutput",

    "DepthStencilState" : {
        "Depth" : { "Enable" : false }
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
