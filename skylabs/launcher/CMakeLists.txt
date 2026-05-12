# launcher
set(CURRENT_TARGET_NAME launcher)

if(ANDROID)
    set(CURRENT_TARGET_NAME main)
    add_library(${CURRENT_TARGET_NAME} SHARED)
    target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE SDL3::SDL3)
else()
    add_executable(${CURRENT_TARGET_NAME})
endif()

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    VS_DEBUGGER_COMMAND ${CMAKE_INSTALL_PREFIX}/$<CONFIG>/$<TARGET_FILE_NAME:${CURRENT_TARGET_NAME}>
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/$<CONFIG>
    # WIN32_EXECUTABLE $<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>> # Do not show console in release
)

set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CURRENT_TARGET_NAME})

target_sources(
    ${CURRENT_TARGET_NAME}
    PRIVATE
        main.c
)

# https://github.com/baldurk/renderdoc/issues/3403
if(UNIX)
    configure_file(
        "${SKYLABS_ROOT_DIR}/cmake/launch_proper_env.sh.in"
        "${CMAKE_CURRENT_BINARY_DIR}/launch_proper_env.sh"
        @ONLY
    )

    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/launch_proper_env.sh" DESTINATION .)
endif()

skylabs_configure_target(${CURRENT_TARGET_NAME} ROOT)
