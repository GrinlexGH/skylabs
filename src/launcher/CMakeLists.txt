# launcher
set(CURRENT_TARGET_NAME launcher)

set(SOURCES
	main.cpp
)

add_executable(${CURRENT_TARGET_NAME} ${SOURCES})

get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
if(NOT CMKR_VS_STARTUP_PROJECT)
	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CURRENT_TARGET_NAME})
endif()


set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.output/${TARGET_ARCHITECTURE}/$<CONFIG>/"
)

foreach(folder ${PUBLIC_FOLDERS})
	target_include_directories(${CURRENT_TARGET_NAME} PRIVATE ${PUBLIC_INCLUDE_DIR}/${folder}/)
endforeach()

target_link_libraries(${CURRENT_TARGET_NAME} SDL2::Core)
target_link_libraries(${CURRENT_TARGET_NAME} SDL2::Main)

