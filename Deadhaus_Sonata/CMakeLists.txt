# Set the LY_PROJECTS_TARGET_NAME property based on the projects.json file
file(READ "${CMAKE_CURRENT_LIST_DIR}/project.json" project_json)
string(JSON project_target_name ERROR_VARIABLE json_error GET ${project_json} "project_name")
if(${json_error})
    message(FATAL_ERROR "Unable to read key 'project_name' from 'project.json'")
endif()
set_property(GLOBAL APPEND PROPERTY LY_PROJECTS_TARGET_NAME ${project_target_name})


add_subdirectory(Gem)
