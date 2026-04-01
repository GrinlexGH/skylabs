# launcher
set(CURRENT_TARGET_NAME launcher)

if(ANDROID)
    add_library(${CURRENT_TARGET_NAME} SHARED)
    target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE SDL3::SDL3)
else()
    add_executable(${CURRENT_TARGET_NAME})
endif()

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SKYLABS_ROOT_OUTPUT_DIRECTORY}
    VS_DEBUGGER_WORKING_DIRECTORY ${SKYLABS_ROOT_DIR}
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    # WIN32_EXECUTABLE $<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>> # Do not show console in release
)

set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CURRENT_TARGET_NAME})

target_sources(
    ${CURRENT_TARGET_NAME}
    PRIVATE
        main.c
)

skylabs_configure_target(${CURRENT_TARGET_NAME})
