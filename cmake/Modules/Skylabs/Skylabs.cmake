# CMake options
if(NOT MINGW)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_AVAILABLE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${LTO_AVAILABLE})
endif()

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

# Set library paths in RUNPATH so that the
# file(GET_RUNTIME_DEPENDENCIES) can find and copy .so files of
# these libraries. It will then restore RUNPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_INSTALL_RPATH "$<IF:$<PLATFORM_ID:Darwin>,@loader_path,\$ORIGIN>")

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

    get_target_property(aliased_target ${target_name} ALIASED_TARGET)
    if(${aliased_target})
        return()
    endif()

    # Source groups
    get_target_property(sources ${target_name} SOURCES)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${sources})

    get_target_property(target_type ${target_name} TYPE)
    set(allowed_types "STATIC_LIBRARY" "MODULE_LIBRARY" "SHARED_LIBRARY" "OBJECT_LIBRARY" "INTERFACE_LIBRARY" "EXECUTABLE")
    if(NOT(target_type IN_LIST allowed_types))
        return()
    endif()

    set(IS_GNU_LIKE $<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>)
    set(IS_MSVC_LIKE $<OR:$<CXX_COMPILER_ID:MSVC>,$<STREQUAL:${CMAKE_CXX_COMPILER_FRONTEND_VARIANT},MSVC>>)
    set(SHOULD_USE_LTO $<AND:$<BOOL:${LTO_AVAILABLE}>,$<NOT:$<CONFIG:Debug>>>)

    # Compile options
    target_compile_options(${target_name} PRIVATE
        $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:
            $<${IS_GNU_LIKE}:
                -Wall -Wextra -Wpedantic
                $<$<AND:$<PLATFORM_ID:Windows>,$<CXX_COMPILER_ID:Clang>>:-fansi-escape-codes>
            >
            $<${IS_MSVC_LIKE}:
                /utf-8 /Zc:__cplusplus /W4
                $<${SHOULD_USE_LTO}:
                    /Gy /Gw
                    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},19.35>>:/Zc:checkGwOdr>
                >
            >
        >
    )

    # Defines
    target_compile_definitions(${target_name} PRIVATE
        $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:DEBUG>

        $<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS VK_USE_PLATFORM_WIN32_KHR UNICODE _UNICODE>
        $<$<PLATFORM_ID:Linux>:PLATFORM_LINUX>
        $<$<PLATFORM_ID:Darwin>:PLATFORM_APPLE>

        $<$<CXX_COMPILER_ID:MSVC>:COMPILER_MSVC>
        $<$<CXX_COMPILER_ID:GNU>:COMPILER_GCC>
        $<$<CXX_COMPILER_ID:Clang>:COMPILER_CLANG>
        $<$<CXX_COMPILER_ID:AppleClang>:COMPILER_CLANG>

        $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:ARCH_64>
        $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>:ARCH_32>

        _CRT_SECURE_NO_WARNINGS
        NOMINMAX

        GLM_FORCE_DEPTH_ZERO_TO_ONE=1
        GLM_FORCE_RADIANS=1
        GLM_ENABLE_EXPERIMENTAL=1

        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_TYPESAFE_CONVERSION=1
        VULKAN_HPP_USE_STD_EXPECTED=1
        VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS=1

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
        COMMENT "Resolving and copying symlinked dependencies..."
    )
endfunction()
