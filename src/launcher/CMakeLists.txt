# launcher
set(CURRENT_TARGET_NAME launcher)

set(SOURCES
    main.cpp
)

add_executable(${CURRENT_TARGET_NAME} ${SOURCES})

if(WIN32)
    set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES WIN32_EXECUTABLE true)
endif()

# set project as startup project
get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
if(NOT CMKR_VS_STARTUP_PROJECT)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CURRENT_TARGET_NAME})
endif()

add_custom_command(
    TARGET ${CURRENT_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${CURRENT_TARGET_NAME}>/bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL2::SDL2>
        $<TARGET_FILE_DIR:${CURRENT_TARGET_NAME}>/bin
    COMMENT "Copying SDL2::SDL2 dll"
)

if(MSVC AND ($<CONFIG:Debug> OR $<CONFIG:RelWithDebInfo>))
    add_custom_command(
        TARGET ${CURRENT_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:SDL2::SDL2>/SDL2.pdb
            $<TARGET_FILE_DIR:${CURRENT_TARGET_NAME}>/bin
        COMMENT "Copying SDL2::SDL2 pdb"
    )
endif()

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${ROOT_OUTPUT_DIRECTORY}/
)

target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE public)
