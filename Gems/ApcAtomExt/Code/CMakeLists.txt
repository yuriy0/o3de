ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

set (APC_ATOM_EXT_COMMON_BUILD_DEPENDENCIES_PRIVATE
	AZ::AzCore
	AZ::AtomCore
	AZ::AzFramework
	Gem::Atom_RHI.Public
	Gem::Atom_RPI.Public
    Gem::ApcUtilities.Static
    Gem::AtomLyIntegration_CommonFeatures.Public
)

ly_add_target(
    NAME ApcAtomExt.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        apcatomext_files.cmake
    PLATFORM_INCLUDE_FILES
		${pal_dir}/apcatomext_gem.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
			${APC_ATOM_EXT_COMMON_BUILD_DEPENDENCIES_PRIVATE}
)

ly_add_target(
    NAME ApcAtomExt ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.ApcAtomExt.b199505e84a344c389606b66bab8226b.v0.1.0
    FILES_CMAKE
        apcatomext_shared_files.cmake
    PLATFORM_INCLUDE_FILES
		${pal_dir}/apcatomext_gem.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::ApcAtomExt.Static
			${APC_ATOM_EXT_COMMON_BUILD_DEPENDENCIES_PRIVATE}
)