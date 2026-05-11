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

set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
set(CMAKE_INSTALL_RPATH "$<IF:$<PLATFORM_ID:Darwin>,@loader_path,\$ORIGIN>")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${CMAKE_BINARY_DIR}/.install")
endif()

get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# Configures a target with common Skylabs project settings:
# - Creates IDE source groups for target sources
# - Applies common compiler and linker options
# - Adds platform, compiler, architecture, and library definitions
# - Configures install rules for binaries, libraries, and runtime dependencies
# - Installs additional files
# - Copies addition files to Android assets if needed
# - Installs optional runtime plugin targets
# - Copies optional runtime plugins into Android jniLibs directory
#
# Options:
#   ROOT               Install executable/library into install root instead of bin/lib
#   RUNTIME_PLUGINS    Additional runtime plugin targets to install/copy
#   INSTALL_FILES      Additional files to install/copy
#   INSTALL_SUBDIR     Destination subdirectory for INSTALL_FILES
function(skylabs_configure_target target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    get_target_property(aliased_target ${target_name} ALIASED_TARGET)
    if(${aliased_target})
        return()
    endif()

    cmake_parse_arguments(ARG "ROOT" "INSTALL_SUBDIR" "RUNTIME_PLUGINS;INSTALL_FILES" ${ARGN})

    # Source groups
    get_target_property(sources ${target_name} SOURCES)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${sources})

    # Copy output files for custom targets
    if(ARG_INSTALL_FILES AND ARG_INSTALL_SUBDIR)
        install(FILES ${ARG_INSTALL_FILES}
            DESTINATION $<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>${ARG_INSTALL_SUBDIR}
        )

        if(ANDROID)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${SKYLABS_ANDROID_ASSETS_DIR}/${ARG_INSTALL_SUBDIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${ARG_INSTALL_FILES} "${SKYLABS_ANDROID_ASSETS_DIR}/${ARG_INSTALL_SUBDIR}"
            )
        endif()
    endif()

    # C++ compiler setup
    get_target_property(target_type ${target_name} TYPE)
    if(NOT("${target_type}" MATCHES "STATIC_LIBRARY|MODULE_LIBRARY|SHARED_LIBRARY|OBJECT_LIBRARY|INTERFACE_LIBRARY|EXECUTABLE"))
        return()
    endif()

    set(IS_GNU_LIKE $<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>)
    set(IS_MSVC_LIKE $<OR:$<CXX_COMPILER_ID:MSVC>,$<STREQUAL:${CMAKE_CXX_COMPILER_FRONTEND_VARIANT},MSVC>>)
    set(SHOULD_USE_LTO $<BOOL:${LTO_AVAILABLE}>)

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

    # Link options
    target_link_options(${target_name} PRIVATE
        $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:
            $<$<CXX_COMPILER_ID:MSVC>:/ignore:4099>
        >
    )

    # Defines
    target_compile_definitions(${target_name} PRIVATE
        $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:DEBUG>

        $<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS UNICODE _UNICODE>
        $<$<PLATFORM_ID:Linux>:PLATFORM_LINUX _XOPEN_SOURCE=700>
        $<$<PLATFORM_ID:Darwin>:PLATFORM_APPLE>
        $<$<PLATFORM_ID:Android>:PLATFORM_ANDROID>

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

        VULKAN_HPP_TYPESAFE_CONVERSION=1
        VULKAN_HPP_USE_STD_EXPECTED=1
    )

    if("${target_type}" MATCHES "OBJECT_LIBRARY|INTERFACE_LIBRARY")
        return()
    endif()

    install(TARGETS ${target_name}
        RUNTIME_DEPENDENCY_SET skylabs_runtime_dependencies
        ARCHIVE DESTINATION $<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>$<IF:$<BOOL:${ARG_ROOT}>,.,lib>
        LIBRARY DESTINATION $<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>$<IF:$<BOOL:${ARG_ROOT}>,.,bin>
        RUNTIME DESTINATION $<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>$<IF:$<BOOL:${ARG_ROOT}>,.,bin>
    )

    install(IMPORTED_RUNTIME_ARTIFACTS ${ARG_RUNTIME_PLUGINS}
        RUNTIME_DEPENDENCY_SET skylabs_runtime_dependencies
        LIBRARY DESTINATION $<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>$<IF:$<BOOL:${ARG_ROOT}>,.,bin>
        RUNTIME DESTINATION $<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>$<IF:$<BOOL:${ARG_ROOT}>,.,bin>
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
endfunction()

include(InstallRequiredSystemLibraries)
install(RUNTIME_DEPENDENCY_SET skylabs_runtime_dependencies
    PRE_EXCLUDE_REGEXES
        "api-ms-win-.*" "ext-ms-.*"
        "libc\.so\..*" "libgcc_s\.so\..*" "libm\.so\..*" "libstdc\\+\\+\.so\..*"
    POST_EXCLUDE_REGEXES
        "^\/lib.*" "^\/usr\/lib.*"
        "C:[\\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\\/].*"
    LIBRARY DESTINATION $<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>bin
    RUNTIME DESTINATION $<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>bin
)

install(DIRECTORY ${SKYLABS_ASSETS_DIR} DESTINATION $<IF:$<BOOL:${IS_MULTI_CONFIG}>,$<CONFIG>/,.>)
