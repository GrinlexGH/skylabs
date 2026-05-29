# launcher
set(CURRENT_TARGET_NAME launcher)

if(ANDROID)
    set(CURRENT_TARGET_NAME main)
    add_library(${CURRENT_TARGET_NAME} SHARED)
    target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE SDL3::SDL3)
else()
    add_executable(${CURRENT_TARGET_NAME})
endif()

target_sources(
    ${CURRENT_TARGET_NAME}
    PRIVATE
        main.c
)

skylabs_configure_target(${CURRENT_TARGET_NAME} RUNTIME_DESTINATION . VS_STARTUP)

# https://github.com/baldurk/renderdoc/issues/3403
if(UNIX)
    configure_file(
        "${SKYLABS_ROOT_DIR}/cmake/launch.sh.in"
        "${CMAKE_CURRENT_BINARY_DIR}/launch.sh"
        @ONLY
    )

    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/launch.sh" DESTINATION .)

    set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES INSTALL_RPATH "\$ORIGIN:\$ORIGIN/lib")
endif()
