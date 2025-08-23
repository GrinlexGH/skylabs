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
    WIN32_EXECUTABLE $<CONFIG:Release> # Do not show console in release
)

# Set project as startup project
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CURRENT_TARGET_NAME})

target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE nowide::nowide)

if(ANDROID)
    target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE SDL3::SDL3)
endif()

# Copy all third party libraries here
set(DLLS_TO_COPY
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE:SDL3_image::SDL3_image>
)

if(ENABLE_STEAM)
    list(APPEND DLLS_TO_COPY
        $<TARGET_FILE:SteamworksSDK::SteamAPI>
        $<TARGET_FILE:SteamworksSDK::AppTicket>
    )
endif()

add_custom_command(
    TARGET ${CURRENT_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${DLLS_TO_COPY} ${DLLS_OUTPUT_DIRECTORY}
    WORKING_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}
    COMMENT "Copying DLLs"
)

if(UNIX)
    # <libname>.so & <libname>.so.0 symlinks for UNIX,
    # because name of TARGET_FILE_NAME is <libname>.so.X.Y.Z
    add_custom_command(
        TARGET ${CURRENT_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE_NAME:SDL3::SDL3> $<PATH:GET_STEM,$<TARGET_FILE_NAME:SDL3::SDL3>>.so
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE_NAME:SDL3::SDL3> $<PATH:GET_STEM,$<TARGET_FILE_NAME:SDL3::SDL3>>.so.0
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE_NAME:SDL3_image::SDL3_image> $<PATH:GET_STEM,$<TARGET_FILE_NAME:SDL3_image::SDL3_image>>.so
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE_NAME:SDL3_image::SDL3_image> $<PATH:GET_STEM,$<TARGET_FILE_NAME:SDL3_image::SDL3_image>>.so.0
        WORKING_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}
        COMMENT "Creating DLL's symlinks"
    )
endif()

if(ANDROID)
    # Copy assets to android apk
    add_custom_command(
        TARGET ${CURRENT_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${SKYLABS_ASSETS_DIRECTORY}
        ${SKYLABS_ANDROID_GENERATED_ASSETS_DIRECTORY}
        WORKING_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}
        COMMENT "Copying assets to apk"
    )
endif()
