ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

set (DHS_COMMON_BUILD_DEPENDENCIES
	AZ::AzGameFramework
	Gem::Atom_AtomBridge.Static
	Gem::EMotionFXStaticLib
	Gem::StartingPointInput.Static
    Gem::ScriptCanvas.Static
)

set (DHS_GAME_GEM_BUILD_DEPENDENCIES
	Gem::EMotionFX.Static
	Gem::PhysX.Static
	Gem::LmbrCentral.Static
)

set (DHS_EDITOR_GEM_BUILD_DEPENDENCIES
	AZ::AzToolsFramework
	Gem::EMotionFX.Editor.Static
	Gem::PhysX.Editor.Static
	Gem::LmbrCentral.Editor.Static
)

ly_add_target(
    NAME Deadhaus_Sonata.Common.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        deadhaus_sonata_files_common.cmake
    PLATFORM_INCLUDE_FILES
		${pal_dir}/deadhaus_sonata_common.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
			.
    BUILD_DEPENDENCIES
        PRIVATE
			${DHS_COMMON_BUILD_DEPENDENCIES}
			${DHS_GAME_GEM_BUILD_DEPENDENCIES}
)
target_precompile_headers(Deadhaus_Sonata.Common.Static PRIVATE ${DHS_PCH_FILE})

ly_add_target(
    NAME Deadhaus_Sonata.Game.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        deadhaus_sonata_files.cmake
    PLATFORM_INCLUDE_FILES
		${pal_dir}/deadhaus_sonata_common.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
			.
    BUILD_DEPENDENCIES
        PRIVATE
			${DHS_COMMON_BUILD_DEPENDENCIES}
			${DHS_GAME_GEM_BUILD_DEPENDENCIES}
)
target_precompile_headers(Deadhaus_Sonata.Game.Static PRIVATE ${DHS_PCH_FILE})

ly_add_target(
    NAME Deadhaus_Sonata ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Deadhaus_Sonata.0ccc66f597594b50814669ffdaf584b3.v0.1.0
    FILES_CMAKE
        deadhaus_sonata_shared_files.cmake
    PLATFORM_INCLUDE_FILES
		${pal_dir}/deadhaus_sonata_common.cmake
		${pal_dir}/deadhaus_sonata_runtime_module.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
			.
    BUILD_DEPENDENCIES
        PRIVATE
			Gem::Deadhaus_Sonata.Common.Static
			# Gem::Deadhaus_Sonata.Game.Static # NB: no files, so no lib generated, and link step fails
			${DHS_COMMON_BUILD_DEPENDENCIES}
			${DHS_GAME_GEM_BUILD_DEPENDENCIES}
)

ly_add_project_dependencies(
    PROJECT_NAME
        Deadhaus_Sonata
    TARGETS 
        Deadhaus_Sonata.GameLauncher
    DEPENDENCIES_FILES 
        runtime_dependencies.cmake
        ${pal_dir}/runtime_dependencies.cmake

)

if (PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME Deadhaus_Sonata.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
			deadhaus_sonata_files.cmake
            deadhaus_sonata_editor_files.cmake
		PLATFORM_INCLUDE_FILES
			${pal_dir}/deadhaus_sonata_common.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                DEADHAUS_SONATA_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
				.
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
				${DHS_COMMON_BUILD_DEPENDENCIES}
				${DHS_EDITOR_GEM_BUILD_DEPENDENCIES}
    )
	target_precompile_headers(Deadhaus_Sonata.Editor.Static PRIVATE ${DHS_PCH_FILE})
	
    ly_add_target(
        NAME Deadhaus_Sonata.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Deadhaus_Sonata.Editor.0ccc66f597594b50814669ffdaf584b3.v0.1.0
        FILES_CMAKE
			deadhaus_sonata_shared_files.cmake
		PLATFORM_INCLUDE_FILES
			${pal_dir}/deadhaus_sonata_common.cmake
			${pal_dir}/deadhaus_sonata_editor_module.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                DEADHAUS_SONATA_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
				.
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
				Gem::Deadhaus_Sonata.Common.Static
                # Gem::Deadhaus_Sonata.Editor.Static # NB: no files, so no lib generated, and link step fails
				${DHS_COMMON_BUILD_DEPENDENCIES}
				${DHS_EDITOR_GEM_BUILD_DEPENDENCIES}
    )
	
    ly_add_project_dependencies(
        PROJECT_NAME
            Deadhaus_Sonata
        TARGETS
            AssetBuilder
            AssetProcessor
            AssetProcessorBatch
            Editor
        DEPENDENCIES_FILES
            tool_dependencies.cmake
            tool_dependencies_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            ${pal_dir}/tool_dependencies.cmake
    )

endif()
