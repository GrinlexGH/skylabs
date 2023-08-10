# launcher
set(CURRENT_TARGET_NAME launcher)

set(SOURCES
	main.cpp
)

add_executable(${CURRENT_TARGET_NAME} WIN32 ${SOURCES})

get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
if(NOT CMKR_VS_STARTUP_PROJECT)
	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CURRENT_TARGET_NAME})
endif()

add_custom_command(TARGET ${CURRENT_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_PATH}/lib/${ARCH}/SDL2.dll
        $<TARGET_FILE_DIR:${CURRENT_TARGET_NAME}>/bin
)

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.output/${ARCH}/$<CONFIG>/"
)
