cmake_minimum_required(VERSION 4.4)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

project(Skylabs HOMEPAGE_URL "https://grinlexstudios.ru" VERSION 2026.0.0 LANGUAGES CXX C)

# Global settings
set(SKYLABS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
    set(SKYLABS_ANDROID_ROOT ${SKYLABS_ROOT_DIR}/android)
    set(SKYLABS_ANDROID_ASSETS_DIR ${SKYLABS_ANDROID_ROOT}/app/src/main/assets)
    set(SKYLABS_ANDROID_JNILIBS_DIR ${SKYLABS_ANDROID_ROOT}/app/src/main/jniLibs/${ANDROID_ABI})
endif()

get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
set(SKYLABS_INSTALL_SUBDIR $<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>)
set(SKYLABS_BUILD_DIR ${CMAKE_BINARY_DIR}/builded/$<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>)

set(SKYLABS_NAME ${PROJECT_NAME})
set(SKYLABS_GAME_NAME ${PROJECT_NAME})
set(SKYLABS_COMPANY "Grinlex")
set(SKYLABS_COPYRIGHT "MIT License. Copyright (c) 2025-present ${SKYLABS_COMPANY}")
set(SKYLABS_HOMEPAGE ${CMAKE_PROJECT_HOMEPAGE_URL})
set(SKYLABS_VERSION ${PROJECT_VERSION})
set(SKYLABS_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(SKYLABS_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(SKYLABS_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(SKYLABS_VERSION_WORD "Melons")

# CMake options
include(CheckIPOSupported)
check_ipo_supported(RESULT LTO_AVAILABLE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${LTO_AVAILABLE})
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET hidden)

set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}bin")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}lib")
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}lib")

set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")

set(CMAKE_DEBUGGER_WORKING_DIRECTORY ${SKYLABS_BUILD_DIR})

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${CMAKE_BINARY_DIR}/.install")
endif()

set(IS_LANG_CCXX $<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>)
set(IS_GNU_LIKE $<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>)
set(IS_MSVC_LIKE $<OR:$<CXX_COMPILER_ID:MSVC>,$<STREQUAL:${CMAKE_CXX_COMPILER_FRONTEND_VARIANT},MSVC>>)
set(IS_LTO_ENABLED $<AND:$<NOT:$<CONFIG:Debug>>,$<BOOL:${LTO_AVAILABLE}>>)

# Disable C4251 because this project is using one runtime
# Disable C4324 because I know what alignas is doing
add_compile_options(
    "$<$<AND:${IS_LANG_CCXX},${IS_GNU_LIKE}>:-Wall;-Wextra;-Wpedantic>"
    "$<$<AND:${IS_LANG_CCXX},$<PLATFORM_ID:Windows>,$<CXX_COMPILER_ID:Clang>>:-fansi-escape-codes>"
    "$<$<AND:${IS_LANG_CCXX},${IS_MSVC_LIKE}>:/utf-8;/Zc:__cplusplus;/Zc:inline;/W4;/wd4251;/wd4324>"
    "$<$<AND:${IS_LANG_CCXX},${IS_MSVC_LIKE},${IS_LTO_ENABLED}>:/Gy;/Gw>"
    "$<$<AND:${IS_LANG_CCXX},${IS_LTO_ENABLED},$<CXX_COMPILER_ID:MSVC>,$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},19.35>>:/Zc:checkGwOdr>"
)

# Disable missing .pdb file warning
add_link_options(
    "$<$<AND:${IS_LANG_CCXX},$<CXX_COMPILER_ID:MSVC>>:/ignore:4099>"
)

add_compile_definitions(
    "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:DEBUG>"

    "$<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS;UNICODE;_UNICODE>"
    "$<$<PLATFORM_ID:Linux>:PLATFORM_LINUX;_XOPEN_SOURCE=700>"
    "$<$<PLATFORM_ID:Darwin>:PLATFORM_APPLE>"
    "$<$<PLATFORM_ID:Android>:PLATFORM_ANDROID>"

    "$<$<CXX_COMPILER_ID:MSVC>:COMPILER_MSVC>"
    "$<$<CXX_COMPILER_ID:GNU>:COMPILER_GCC>"
    "$<$<CXX_COMPILER_ID:Clang>:COMPILER_CLANG>"
    "$<$<CXX_COMPILER_ID:AppleClang>:COMPILER_CLANG>"

    "$<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:ARCH_64>"
    "$<$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>:ARCH_32>"

    "NOMINMAX=1"
    "GLM_FORCE_DEPTH_ZERO_TO_ONE=1"
    "GLM_FORCE_RADIANS=1"
    "GLM_ENABLE_EXPERIMENTAL=1"
    "VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0"
    "VULKAN_HPP_TYPESAFE_CONVERSION=1"
    "VULKAN_HPP_USE_STD_EXPECTED=1"
    "VK_NO_PROTOTYPES=1"
)

# Functions to setup targets
function(skylabs_target_setup_ide target_name)
    set(options VS_STARTUP)
    cmake_parse_arguments(ARG "${options}" "" "" ${ARGN})

    get_target_property(TARGET_TYPE ${target_name} TYPE)
    get_target_property(TARGET_SOURCES ${target_name} SOURCES)

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${TARGET_SOURCES})

    if(ARG_VS_STARTUP AND ("${TARGET_TYPE}" STREQUAL "EXECUTABLE") AND (CMAKE_GENERATOR MATCHES "Visual Studio .*"))
        set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${target_name})
    endif()
endfunction()

function(skylabs_target_setup_install target_name)
    set(oneValueArgs RUNTIME_DESTINATION ARCHIVE_DESTINATION LIBRARY_DESTINATION)
    set(multiValueArgs RUNTIME_PLUGINS)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(runtime_dest "bin")
    set(archive_dest "lib")
    set(library_dest "lib")

    if(ARG_RUNTIME_DESTINATION)
        set(runtime_dest ${ARG_RUNTIME_DESTINATION})
    endif()

    if(ARG_ARCHIVE_DESTINATION)
        set(archive_dest ${ARG_ARCHIVE_DESTINATION})
    endif()

    if(ARG_LIBRARY_DESTINATION)
        set(library_dest ${ARG_LIBRARY_DESTINATION})
    endif()

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}${runtime_dest}"
        PDB_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}${runtime_dest}"
        ARCHIVE_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}${archive_dest}"
        COMPILE_PDB_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}${archive_dest}"
        LIBRARY_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}${library_dest}"
    )

    set(RUNTIME_DEPENDENCIES_ARGS "")
    if(NOT CMAKE_CROSSCOMPILING)
        set(RUNTIME_LOOKUP_DIRECTORIES "")
        if(WIN32)
            list(APPEND RUNTIME_LOOKUP_DIRECTORIES "${CONAN_RUNTIME_LIB_DIRS}")
            cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH CXX_COMPILER_BIN_DIR)
            list(APPEND RUNTIME_LOOKUP_DIRECTORIES "${CXX_COMPILER_BIN_DIR}")
            cmake_path(GET CMAKE_C_COMPILER PARENT_PATH C_COMPILER_BIN_DIR)
            list(APPEND RUNTIME_LOOKUP_DIRECTORIES "${C_COMPILER_BIN_DIR}")
            list(APPEND RUNTIME_LOOKUP_DIRECTORIES "${SKYLABS_BUILD_DIR}")
            list(APPEND RUNTIME_LOOKUP_DIRECTORIES "${SKYLABS_BUILD_DIR}bin")
            list(APPEND RUNTIME_LOOKUP_DIRECTORIES "${SKYLABS_BUILD_DIR}lib")
        endif()

        set(RUNTIME_DEPENDENCIES_ARGS
            RUNTIME_DEPENDENCIES
            DIRECTORIES ${RUNTIME_LOOKUP_DIRECTORIES}
            PRE_EXCLUDE_REGEXES
                "api-ms-win-.*" "ext-ms-.*"
                "libc\.so\..*" "libgcc_s\.so\..*" "libm\.so\..*" "libstdc\\+\\+\.so\..*"
            POST_EXCLUDE_REGEXES
                "^\/lib.*" "^\/usr\/lib.*"
                "C:[\\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\\/].*"
            POST_INCLUDE_REGEXES
                "[Vv][Cc][Rr][Uu][Nn][Tt][Ii][Mm][Ee].*" "[Mm][Ss][Vv][Cc][Pp].*"
        )
    endif()

    install(
        FILES $<$<BOOL:${MSVC}>:$<TARGET_PDB_FILE:${target_name}>>
        DESTINATION ${SKYLABS_INSTALL_SUBDIR}${runtime_dest}
        OPTIONAL
    )

    install(TARGETS ${target_name}
        ${RUNTIME_DEPENDENCIES_ARGS}
        ARCHIVE DESTINATION ${SKYLABS_INSTALL_SUBDIR}${archive_dest}
        LIBRARY DESTINATION ${SKYLABS_INSTALL_SUBDIR}${library_dest}
        RUNTIME DESTINATION ${SKYLABS_INSTALL_SUBDIR}${runtime_dest}
    )

    if(ARG_RUNTIME_PLUGINS)
        install(IMPORTED_RUNTIME_ARTIFACTS ${ARG_RUNTIME_PLUGINS}
            RUNTIME_DEPENDENCY_SET
            LIBRARY DESTINATION ${SKYLABS_INSTALL_SUBDIR}${library_dest}
            RUNTIME DESTINATION ${SKYLABS_INSTALL_SUBDIR}${runtime_dest}
        )

        if(ANDROID)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E
                    make_directory "${SKYLABS_ANDROID_JNILIBS_DIR}"
            )
            foreach(plugin_target IN LISTS ARG_RUNTIME_PLUGINS)
                add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:${plugin_target}> "${SKYLABS_ANDROID_JNILIBS_DIR}"
                )
            endforeach()
        endif()
    endif()
endfunction()

# skylabs_install_directory(<target>
#     DIRECTORIES <directories>... DESTINATION <path>
# )
# Parameters:
#   DIRECTORIES                 List of directories to install with target
#   DESTINATION                 Subdirectory of install destination
function(skylabs_install_directories target_name)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs DIRECTORIES)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    install(DIRECTORY ${ARG_DIRECTORIES}
        DESTINATION ${SKYLABS_INSTALL_SUBDIR}${ARG_DESTINATION}
    )

    if(ANDROID)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${SKYLABS_ANDROID_ASSETS_DIR}/${ARG_DESTINATION}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                ${ARG_DIRECTORIES} "${SKYLABS_ANDROID_ASSETS_DIR}/${ARG_DESTINATION}"
        )
    endif()
endfunction()

# skylabs_install_files(<target>
#     FILES <files>... DESTINATION <path>
# )
# Parameters:
#   FILES                       List of files to install with target
#   DESTINATION                 Subdirectory of install destination
function(skylabs_install_files target_name)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs FILES)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    install(FILES ${ARG_FILES}
        DESTINATION ${SKYLABS_INSTALL_SUBDIR}${ARG_DESTINATION}
    )

    if(ANDROID)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${SKYLABS_ANDROID_ASSETS_DIR}/${ARG_DESTINATION}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${ARG_FILES} "${SKYLABS_ANDROID_ASSETS_DIR}/${ARG_DESTINATION}"
        )
    endif()
endfunction()

# skylabs_configure_target(<target>
#     [WIN32_IN_RELEASE] [VS_STARTUP]
#     [RUNTIME_DESTINATION <path>]
#     [RUNTIME_PLUGINS <targets>...]
#     [CUSTOM_FILES <files>...]
#     [CUSTOM_FILES_DESTINATION <path>]
# )
# Parameters:
#   WIN32_IN_RELEASE            Set WIN32_EXECUTABLE target property in release mode
#   VS_STARTUP                  Set target as Visual Studio default startup project
#   RUNTIME_DESTINATION         Subdirectory of install destination for runtime artifacts
#   ARCHIVE_DESTINATION         Subdirectory of install destination for linking artifacts
#   LIBRARY_DESTINATION         Subdirectory of install destination for library artifacts
#   RUNTIME_PLUGINS             Additional targets for install as IMPORTED_RUNTIME_ARTIFACTS
function(skylabs_configure_target target_name)
    set(options WIN32_IN_RELEASE VS_STARTUP)
    set(oneValueArgs RUNTIME_DESTINATION ARCHIVE_DESTINATION LIBRARY_DESTINATION)
    set(multiValueArgs RUNTIME_PLUGINS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(function_args "")
    if(ARG_VS_STARTUP)
        list(APPEND function_args VS_STARTUP)
    endif()

    skylabs_target_setup_ide(${target_name} ${function_args})

    get_target_property(TARGET_TYPE ${target_name} TYPE)
    if(NOT("${TARGET_TYPE}" MATCHES "STATIC_LIBRARY|MODULE_LIBRARY|SHARED_LIBRARY|OBJECT_LIBRARY|EXECUTABLE"))
        return()
    endif()

    if(${ARG_WIN32_IN_RELEASE} AND ("${TARGET_TYPE}" STREQUAL "EXECUTABLE"))
        set_target_properties(${target_name} PROPERTIES WIN32_EXECUTABLE $<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>)
    endif()

    set(function_args "")
    if(ARG_RUNTIME_DESTINATION)
        list(APPEND function_args RUNTIME_DESTINATION "${ARG_RUNTIME_DESTINATION}")
    endif()

    if(ARG_ARCHIVE_DESTINATION)
        list(APPEND function_args ARCHIVE_DESTINATION "${ARG_ARCHIVE_DESTINATION}")
    endif()

    if(ARG_LIBRARY_DESTINATION)
        list(APPEND function_args LIBRARY_DESTINATION "${ARG_LIBRARY_DESTINATION}")
    endif()

    if(ARG_RUNTIME_PLUGINS)
        list(APPEND function_args RUNTIME_PLUGINS ${ARG_RUNTIME_PLUGINS})
    endif()

    skylabs_target_setup_install(${target_name} ${function_args})
endfunction()

# Install assets
install(DIRECTORY ${SKYLABS_ROOT_DIR}/assets DESTINATION ${SKYLABS_INSTALL_SUBDIR}.)

# Third party libraries
if(ANDROID)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
endif()

find_package(Boost COMPONENTS nowide container CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(frozen CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(slang CONFIG REQUIRED)
find_package(stb CONFIG REQUIRED)
find_package(SteamworksSDK CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(VulkanMemoryAllocator-Hpp CONFIG REQUIRED)

# Skylabs
include_directories("${PROJECT_SOURCE_DIR}")
add_subdirectory(skylabs)
