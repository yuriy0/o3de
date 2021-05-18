set (LY_LINK_OPTIONS PRIVATE
	Winmm.lib
)

# Mainly for backwards compatability
set (LY_COMPILE_OPTIONS
	PRIVATE
		/wd4100 # unused local parameter
		/wd4458 # declaration of 'name' hides class member
	PUBLIC
		/bigobj
)