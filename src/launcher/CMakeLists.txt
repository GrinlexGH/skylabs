# launcher
set(CURRENT_TARGET_NAME launcher)

set(SOURCES
    main.cpp
)

add_executable(${CURRENT_TARGET_NAME} ${SOURCES})

foreach(FILE IN LISTS SOURCES)
    get_filename_component(SOURCE_PATH "${FILE}" PATH)
    string(REPLACE "/" "\\" SOURCE_PATH_GROUP "${SOURCE_PATH}")
    source_group("Source Files\\${SOURCE_PATH_GROUP}" FILES "${FILE}")
endforeach()

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBS_OUTPUT_DIRECTORY}/
    RUNTIME_OUTPUT_DIRECTORY ${ROOT_OUTPUT_DIRECTORY}/
    LIBRARY_OUTPUT_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}/
)

# Set project as startup project
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CURRENT_TARGET_NAME})

if(MSVC)
    target_link_options(${CURRENT_TARGET_NAME} PRIVATE "$<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>")
endif()

target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE nowide::nowide)

# Copy all third party libraries here
add_custom_command(
    TARGET ${CURRENT_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE:SDL3_image::SDL3_image>
    $<TARGET_FILE:SteamworksSDK::SteamworksSDK>
    $<TARGET_FILE:SteamworksSDK::AppTicket>
    ${DLLS_OUTPUT_DIRECTORY}
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    $<TARGET_FILE_NAME:SDL3::SDL3> $<PATH:GET_STEM,$<TARGET_FILE_NAME:SDL3::SDL3>>.so
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    $<TARGET_FILE_NAME:SDL3::SDL3> $<PATH:GET_STEM,$<TARGET_FILE_NAME:SDL3::SDL3>>.so.0
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    $<TARGET_FILE_NAME:SDL3_image::SDL3_image> $<PATH:GET_STEM,$<TARGET_FILE_NAME:SDL3_image::SDL3_image>>.so
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    $<TARGET_FILE_NAME:SDL3_image::SDL3_image> $<PATH:GET_STEM,$<TARGET_FILE_NAME:SDL3_image::SDL3_image>>.so.0
    WORKING_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}
    COMMENT "Copying libraries dll"
)
