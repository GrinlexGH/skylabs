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

skylabs_configure_target(${CURRENT_TARGET_NAME} RUNTIME_DESTINATION $<IF:$<PLATFORM_ID:Windows>,.,bin> VS_STARTUP)
