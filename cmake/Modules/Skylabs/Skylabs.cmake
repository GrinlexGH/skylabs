# All shared project setting

# Variables
set(SKYLABS_ROOT_DIR ${PROJECT_SOURCE_DIR})
set(SKYLABS_ASSETS_DIR ${SKYLABS_ROOT_DIR}/assets)

set(SKYLABS_ROOT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>)
set(SKYLABS_BIN_OUTPUT_DIRECTORY ${SKYLABS_ROOT_OUTPUT_DIRECTORY}/bin)
set(SKYLABS_LIB_OUTPUT_DIRECTORY ${SKYLABS_ROOT_OUTPUT_DIRECTORY}/lib)
set(SKYLABS_SHADER_OUTPUT_DIRECTORY ${SKYLABS_ROOT_OUTPUT_DIRECTORY}/shaders)

set(SKYLABS_NAME ${PROJECT_NAME})
set(SKYLABS_GAME_NAME ${PROJECT_NAME})
set(SKYLABS_COMPANY "Grinlex Stydios")
set(SKYLABS_COPYRIGHT "MIT License. Copyright (c) 2025-present ${SKYLABS_COMPANY}")
set(SKYLABS_HOMEPAGE ${CMAKE_PROJECT_HOMEPAGE_URL})
set(SKYLABS_VERSION ${PROJECT_VERSION})
set(SKYLABS_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(SKYLABS_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(SKYLABS_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(SKYLABS_VERSION_WORD "Melons")

option(SKYLABS_TRY_LTO "Try to enable LTO" ON)

# Enabling LTO
if(SKYLABS_TRY_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_AVAILABLE)
    if(LTO_AVAILABLE)
        message(STATUS "LTO has been enabled")
    endif()
endif()

# CMake settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET hidden)

set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Set library paths in RUNPATH so that the
# file(GET_RUNTIME_DEPENDENCIES) can find and copy .so files of
# these libraries. It will then restore RUNPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_INSTALL_RPATH "@loader_path")
else()
    set(CMAKE_INSTALL_RPATH "\$ORIGIN")
endif()

set(CMAKE_PDB_OUTPUT_DIRECTORY ${SKYLABS_BIN_OUTPUT_DIRECTORY})
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${SKYLABS_BIN_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SKYLABS_BIN_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SKYLABS_BIN_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SKYLABS_LIB_OUTPUT_DIRECTORY})

# Setup skylabs target
function(skylabs_configure_target target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    get_property(aliased_target TARGET ${target_name} PROPERTY ALIASED_TARGET)
    if(${aliased_target})
        return()
    endif()

    # Source groups
    get_target_property(sources ${target_name} SOURCES)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${sources})

    # Compile options
    get_target_property(target_type ${target_name} TYPE)
    set(allowed_types "STATIC_LIBRARY" "MODULE_LIBRARY" "SHARED_LIBRARY" "OBJECT_LIBRARY" "INTERFACE_LIBRARY" "EXECUTABLE")
    if(NOT(target_type IN_LIST allowed_types))
        return()
    endif()

    set(IS_GNU_LIKE $<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>)
    set(IS_MSVC_LIKE $<OR:$<CXX_COMPILER_ID:MSVC>,$<STREQUAL:${CMAKE_CXX_COMPILER_FRONTEND_VARIANT},MSVC>>)
    set(SHOULD_USE_LTO $<AND:$<BOOL:${LTO_AVAILABLE}>,$<NOT:$<CONFIG:Debug>>>)

    target_compile_options(${target_name} PRIVATE
        $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:
            $<$<BOOL:${IS_GNU_LIKE}>:
                -Wall -Wextra -Wpedantic
                $<$<AND:$<BOOL:${WIN32}>,$<CXX_COMPILER_ID:Clang>>:-fansi-escape-codes>
            >
            $<$<BOOL:${IS_MSVC_LIKE}>:
                /utf-8 /Zc:__cplusplus /W4
                $<$<BOOL:${SHOULD_USE_LTO}>:
                    /Gy /Gw
                    $<$<CXX_COMPILER_ID:MSVC>:
                        $<$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},19.35>:/Zc:checkGwOdr>
                    >
                >
            >
        >
    )

    # Defines
    target_compile_definitions(${target_name} PRIVATE
        $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:RelWithDebInfo>:DEBUG>
        $<$<BOOL:${WIN32}>:PLATFORM_WINDOWS VK_USE_PLATFORM_WIN32_KHR UNICODE _UNICODE>
        $<$<BOOL:${UNIX}>:PLATFORM_UNIX>
        $<$<BOOL:${LINUX}>:PLATFORM_LINUX>
        $<$<BOOL:${APPLE}>:PLATFORM_APPLE>
        $<$<OR:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>:COMPILER_MSVC>
        $<$<OR:$<C_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:GNU>>:COMPILER_GCC>
        $<$<OR:$<C_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:Clang>>:COMPILER_Clang>
        $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:ARCH_64>
        $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>:ARCH_32>

        _CRT_SECURE_NO_WARNINGS NOMINMAX
        GLM_FORCE_DEPTH_ZERO_TO_ONE=1
        GLM_FORCE_RADIANS=1
        GLM_ENABLE_EXPERIMENTAL=1
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_TYPESAFE_CONVERSION=1
        VULKAN_HPP_USE_STD_EXPECTED=1
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=0
        VP_USE_OBJECT=1
    )

    # DLL copying
    set(allowed_types "EXECUTABLE" "SHARED_LIBRARY" "MODULE_LIBRARY")
    if(NOT(target_type IN_LIST allowed_types))
        return()
    endif()

    if(WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${target_name}> $<TARGET_RUNTIME_DLLS:${target_name}>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs to ${target_name} output directory"
        )

        # Get compilers path
        cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH CXX_COMPILER_DIR)
        cmake_path(GET CMAKE_C_COMPILER PARENT_PATH C_COMPILER_DIR)
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        "\"-DOUTPUT_DIR=$<TARGET_FILE_DIR:${target_name}>\""
        "\"-DDIRECTORIES=${CXX_COMPILER_DIR};${C_COMPILER_DIR}\""
        "\"-D${target_type}=$<TARGET_FILE:${target_name}>\""
        -P ${CMAKE_SOURCE_DIR}/cmake/CopyDeps.cmake
        ${cmd}
        COMMENT "Resolving and copying symlinked dependencies..."
    )
endfunction()
