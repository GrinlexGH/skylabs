# launcher
if(ANDROID)
    set(CURRENT_TARGET_NAME main)
else()
    set(CURRENT_TARGET_NAME launcher)
endif()

set(SOURCES
    main.cpp
)

if(ANDROID)
    add_library(${CURRENT_TARGET_NAME} SHARED ${SOURCES})
else()
    add_executable(${CURRENT_TARGET_NAME} ${SOURCES})
endif()

foreach(source IN LISTS SOURCES)
    cmake_path(GET source PARENT_PATH source_directory)
    source_group("Source Files/${source_directory}" FILES "${source}")
endforeach()

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SKYLABS_ROOT_OUTPUT_DIRECTORY}
    VS_DEBUGGER_WORKING_DIRECTORY ${SKYLABS_ROOT_OUTPUT_DIRECTORY}
    WIN32_EXECUTABLE $<CONFIG:Release> # Do not show console in release
)

add_dependencies(${CURRENT_TARGET_NAME} core)

# Set project as startup project
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CURRENT_TARGET_NAME})

deps_target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE nowide::nowide)

if(ANDROID)
    target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE SDL3::SDL3)
endif()

if(ANDROID)
    # Copy assets to android apk
    add_custom_command(
        TARGET ${CURRENT_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${SKYLABS_ASSETS_DIR}
        ${SKYLABS_ANDROID_GENERATED_ASSETS_DIR}
        COMMENT "Copying assets to apk"
    )
endif()
