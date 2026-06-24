cmake_minimum_required(VERSION 3.28...4.3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

project(Skylabs HOMEPAGE_URL "https://grinlexstudios.ru" VERSION 0.0.0 LANGUAGES CXX C)

# Global settings
set(SKYLABS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SKYLABS_ASSETS_DIR ${SKYLABS_ROOT_DIR}/assets)

if(ANDROID)
    set(SKYLABS_ANDROID_ROOT ${SKYLABS_ROOT_DIR}/android)
    set(SKYLABS_ANDROID_ASSETS_DIR ${SKYLABS_ANDROID_ROOT}/app/src/main/assets)
    set(SKYLABS_ANDROID_JNILIBS_DIR ${SKYLABS_ANDROID_ROOT}/app/src/main/jniLibs/${ANDROID_ABI})
endif()

set(SKYLABS_INSTALL_SUBDIR $<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>)

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

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_CXX_MODULE_STD ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET hidden)

set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
set(CMAKE_INSTALL_RPATH "\$ORIGIN")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${CMAKE_BINARY_DIR}/.install")
endif()

set(IS_LANG_CCXX $<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>)
set(IS_GNU_LIKE $<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>)
set(IS_MSVC_LIKE $<OR:$<CXX_COMPILER_ID:MSVC>,$<STREQUAL:${CMAKE_CXX_COMPILER_FRONTEND_VARIANT},MSVC>>)

add_compile_options(
    "$<$<AND:${IS_LANG_CCXX},${IS_GNU_LIKE}>:-Wall;-Wextra;-Wpedantic>"
    "$<$<AND:${IS_LANG_CCXX},$<PLATFORM_ID:Windows>,$<CXX_COMPILER_ID:Clang>>:-fansi-escape-codes;-Wno-reserved-module-identifier>"
    "$<$<AND:${IS_LANG_CCXX},${IS_MSVC_LIKE}>:/utf-8;/Zc:__cplusplus;/Zc:preprocessor;/W4>"
    "$<$<AND:${IS_LANG_CCXX},${IS_MSVC_LIKE},$<BOOL:${LTO_AVAILABLE}>>:/Gy;/Gw>"
    "$<$<AND:${IS_LANG_CCXX},$<BOOL:${LTO_AVAILABLE}>,$<CXX_COMPILER_ID:MSVC>,$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},19.35>>:/Zc:checkGwOdr>"
)

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

    "_CRT_SECURE_NO_WARNINGS=1"
    "NOMINMAX=1"
    "GLM_FORCE_DEPTH_ZERO_TO_ONE=1"
    "GLM_FORCE_RADIANS=1"
    "GLM_ENABLE_EXPERIMENTAL=1"
    "VULKAN_HPP_TYPESAFE_CONVERSION=1"
    "VULKAN_HPP_USE_STD_EXPECTED=1"
    "VK_NO_PROTOTYPES=1"
    "FMT_IMPORT_STD=1"
)

# Functions to setup targets
function(skylabs_target_setup_ide target_name vs_startup runtime_destination)
    get_target_property(TARGET_TYPE ${target_name} TYPE)
    get_target_property(TARGET_SOURCES ${target_name} SOURCES)

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${TARGET_SOURCES})

    if("${TARGET_TYPE}" STREQUAL "EXECUTABLE")
        set_target_properties(${target_name} PROPERTIES
            VS_DEBUGGER_COMMAND ${CMAKE_INSTALL_PREFIX}/${SKYLABS_INSTALL_SUBDIR}/${runtime_destination}/$<TARGET_FILE_NAME:${target_name}>
            VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${SKYLABS_INSTALL_SUBDIR}
        )

        if(vs_startup)
            set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${target_name})
        endif()
    endif()
endfunction()

function(skylabs_target_setup_install target_name)
    cmake_parse_arguments(ARG "" "RUNTIME_DESTINATION" "RUNTIME_PLUGINS" ${ARGN})

    get_target_property(TARGET_TYPE ${target_name} TYPE)
    if(UNIX AND "${TARGET_TYPE}" STREQUAL "EXECUTABLE")
        set_target_properties(${target_name} PROPERTIES INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../lib")
    endif()

    set(runtime_dest "bin")
    if(WIN32 AND ("${TARGET_TYPE}" STREQUAL "MODULE_LIBRARY"))
        set(library_dest "bin")
    else()
        set(library_dest "lib")
    endif()

    if(ARG_RUNTIME_DESTINATION)
        set(runtime_dest ${ARG_RUNTIME_DESTINATION})
        set(library_dest ${ARG_RUNTIME_DESTINATION})
    endif()

    set(RUNTIME_LOOKUP_DIRECTORIES "")
    if(WIN32)
        list(APPEND RUNTIME_LOOKUP_DIRECTORIES "${CONAN_RUNTIME_LIB_DIRS}")
        cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH CXX_COMPILER_BIN_DIR)
        list(APPEND RUNTIME_LOOKUP_DIRECTORIES "${CXX_COMPILER_BIN_DIR}")
        cmake_path(GET CMAKE_C_COMPILER PARENT_PATH C_COMPILER_BIN_DIR)
        list(APPEND RUNTIME_LOOKUP_DIRECTORIES "${C_COMPILER_BIN_DIR}")
    endif()

    set(RUNTIME_DEPENDENCIES_ARGS "")
    if(NOT CMAKE_CROSSCOMPILING)
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

    install(TARGETS ${target_name}
        ${RUNTIME_DEPENDENCIES_ARGS}
        ARCHIVE DESTINATION ${SKYLABS_INSTALL_SUBDIR}${library_dest}
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

function(skylabs_target_setup_install_custom_files target_name)
    cmake_parse_arguments(ARG "" "DESTINATION" "FILES" ${ARGN})

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

# Parameters:
#   WIN32_IN_RELEASE            Set WIN32_EXECUTABLE target property in release mode
#   VS_STARTUP                  Set target as Visual Studio default startup project
#   RUNTIME_DESTINATION         Subdir of install destination
#   RUNTIME_PLUGINS             Additional targets for install as IMPORTED_RUNTIME_ARTIFACTS
#   CUSTOM_FILES                List of custom files to install with target
#   CUSTOM_FILES_DESTINATION    Subdir of install destination
function(skylabs_configure_target target_name)
    set(options WIN32_IN_RELEASE VS_STARTUP)
    set(oneValueArgs RUNTIME_DESTINATION CUSTOM_FILES_DESTINATION)
    set(multiValueArgs RUNTIME_PLUGINS CUSTOM_FILES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    skylabs_target_setup_ide(${target_name} "${ARG_VS_STARTUP}" "${ARG_RUNTIME_DESTINATION}")

    if(ARG_CUSTOM_FILES AND ARG_CUSTOM_FILES_DESTINATION)
        skylabs_target_setup_install_custom_files(${target_name}
            FILES ${ARG_CUSTOM_FILES}
            DESTINATION "${ARG_CUSTOM_FILES_DESTINATION}"
        )
    endif()

    get_target_property(TARGET_TYPE ${target_name} TYPE)
    if(NOT("${TARGET_TYPE}" MATCHES "STATIC_LIBRARY|MODULE_LIBRARY|SHARED_LIBRARY|OBJECT_LIBRARY|EXECUTABLE"))
        return()
    endif()

    if(${ARG_WIN32_IN_RELEASE} AND ("${TARGET_TYPE}" STREQUAL "EXECUTABLE"))
        set_target_properties(${target_name} PROPERTIES WIN32_EXECUTABLE $<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>)
    endif()

    set(install_args "")
    if(ARG_RUNTIME_DESTINATION)
        list(APPEND install_args RUNTIME_DESTINATION "${ARG_RUNTIME_DESTINATION}")
    endif()
    if(ARG_RUNTIME_PLUGINS)
        list(APPEND install_args RUNTIME_PLUGINS ${ARG_RUNTIME_PLUGINS})
    endif()

    skylabs_target_setup_install(${target_name} ${install_args})
endfunction()

install(DIRECTORY ${SKYLABS_ASSETS_DIR} DESTINATION ${SKYLABS_INSTALL_SUBDIR}.)

# Third party libraries
find_package(Boost COMPONENTS nowide container CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(frozen CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(slang CONFIG REQUIRED)
find_package(stb CONFIG REQUIRED)
find_package(SteamworksSDK CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# vulkan headers module
find_package(VulkanHeaders CONFIG REQUIRED)
get_target_property(VULKAN_INCLUDE_DIR Vulkan::Headers INTERFACE_INCLUDE_DIRECTORIES)
add_library(Vulkan-HppModule OBJECT)
target_sources(Vulkan-HppModule PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS ${VULKAN_INCLUDE_DIR}
    FILES
        ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.cppm
        ${VULKAN_INCLUDE_DIR}/vulkan/vulkan_video.cppm
)
target_link_libraries(Vulkan-HppModule PUBLIC Vulkan::Headers)

# vulkan memory allocator module
find_package(VulkanMemoryAllocator-Hpp CONFIG REQUIRED)
get_target_property(VMA_HPP_INCLUDE_DIR VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp INTERFACE_INCLUDE_DIRECTORIES)
add_library(VulkanMemoryAllocator-HppModule OBJECT)
target_sources(VulkanMemoryAllocator-HppModule PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS ${VMA_HPP_INCLUDE_DIR}
    FILES
        ${VMA_HPP_INCLUDE_DIR}/vk_mem_alloc.cppm
)
set_source_files_properties(${VMA_HPP_INCLUDE_DIR}/vk_mem_alloc.cppm PROPERTIES COMPILE_OPTIONS "$<${IS_GNU_LIKE}:-w>")
target_link_libraries(VulkanMemoryAllocator-HppModule PUBLIC VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp Vulkan-HppModule)

# glm module
find_package(glm CONFIG REQUIRED)
get_target_property(GLM_INCLUDE_DIR glm::glm-header-only INTERFACE_INCLUDE_DIRECTORIES)
add_library(glm-HppModule OBJECT)
target_sources(glm-HppModule PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS ${GLM_INCLUDE_DIR}
    FILES
        ${GLM_INCLUDE_DIR}/glm/glm.cppm
)
target_link_libraries(glm-HppModule PUBLIC glm::glm-header-only)

# fmt module
find_package(fmt CONFIG REQUIRED)
get_target_property(FMT_INCLUDE_DIR fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
add_library(fmt-HppModule OBJECT)
target_sources(fmt-HppModule PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS ${FMT_INCLUDE_DIR}
    FILES
        ${FMT_INCLUDE_DIR}/fmt/src/fmt.cc
)
target_link_libraries(fmt-HppModule PUBLIC fmt::fmt-header-only)

# Skylabs
include_directories("${PROJECT_SOURCE_DIR}")
add_subdirectory(skylabs)
