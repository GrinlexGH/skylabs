# launcher
if(ANDROID)
    set(CURRENT_TARGET_NAME main)
    add_library(${CURRENT_TARGET_NAME} SHARED ${SOURCES})
    target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE SDL3::SDL3)

    # Copy assets to android apk
    add_custom_command(
        TARGET ${CURRENT_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${SKYLABS_ASSETS_DIR}
        ${SKYLABS_ANDROID_GENERATED_ASSETS_DIR}
        COMMENT "Copying assets to apk"
    )
else()
    set(CURRENT_TARGET_NAME launcher)
    add_executable(${CURRENT_TARGET_NAME} ${SOURCES})
endif()

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SKYLABS_ROOT_OUTPUT_DIRECTORY}
    VS_DEBUGGER_WORKING_DIRECTORY ${SKYLABS_ROOT_OUTPUT_DIRECTORY}
    WIN32_EXECUTABLE $<CONFIG:Release> # Do not show console in release
)

set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CURRENT_TARGET_NAME})

target_sources(
    ${CURRENT_TARGET_NAME}
    PRIVATE
        main.cpp
)

add_dependencies(${CURRENT_TARGET_NAME} core)
target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE Boost::nowide)

skylabs_configure_target(${CURRENT_TARGET_NAME})
