# ===============================================================================================
# Dependency Installer for CMake Projects
# Author: Grinlex
# -----------------------------------------------------------------------------------------------
# Overview:
#
# This module provides an autonomous way to build and install external dependencies during
# CMake configuration. It allows a project to remain self-contained without requiring
# preinstalled SDKs or system-wide packages.
#
# -----------------------------------------------------------------------------------------------
# Usage example:
#
#   # linux-arm.cmake toolchain file
#   set(CMAKE_SYSTEM_NAME Linux)    # DEPS_TARGET_SYSTEM will be equal to CMAKE_SYSTEM_NAME
#   set(CMAKE_SYSTEM_PROCESSOR arm) # DEPS_TARGET_ARCH will be equal to CMAKE_SYSTEM_PROCESSOR
#
#   set(tools /home/devel/gcc-4.7-linaro-rpi-gnueabihf)
#   set(CMAKE_C_COMPILER ${tools}/bin/arm-linux-gnueabihf-gcc)
#   set(CMAKE_CXX_COMPILER ${tools}/bin/arm-linux-gnueabihf-g++)
#
#   set(DEPS_OUT_SUBDIR "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-libstdcxx")
#
#   include(deps)
#   deps_append_cmake_define(CMAKE_SYSTEM_NAME)
#   deps_append_cmake_define(CMAKE_SYSTEM_PROCESSOR)
#   deps_append_cmake_define(CMAKE_C_COMPILER)
#   deps_append_cmake_define(CMAKE_CXX_COMPILER)
#
#   # CMakeLists.txt
#   cmake_minimum_required(VERSION 3.26)
#   project("Skylabs" LANGUAGES CXX C)
#
#   list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")
#   include(Deps)
#
#   deps_append_cmake_define(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
#   if(ANDROID)
#       deps_append_cmake_define(ANDROID_ABI)
#       deps_append_cmake_define(CMAKE_ANDROID_ARCH_ABI)
#   endif()
#
#   deps_add_cmake_project("SDL" INSTALL_SUBDIR "SDL3" CMAKE_ARGS -DSDL_TEST_LIBRARY=OFF)
#   deps_add_header_only("tinyobjloader" HEADERS "tiny_obj_loader.h")
#   deps_add_manual_install(
#       "SteamworksSDK"
#       INSTALL_SUBDIR "SteamworksSDK"
#       RULES
#         "redistributable_bin/**/*.dll"      "bin"
#         "public/steam/lib/**/*.dll"         "bin"
#         "public/steam/*.h"                  "include/steam"
#         "redistributable_bin/**/*.lib"      "lib"
#         "redistributable_bin/**/*.so"       "lib"
#         "redistributable_bin/**/*.dylib"    "lib"
#         "public/steam/lib/**/*.lib"         "lib"
#         "public/steam/lib/**/*.so"          "lib"
#         "public/steam/lib/**/*.dylib"       "lib"
#   )
#   deps_build_all()
#
#   include_directories(SYSTEM "${DEPS_HEADER_ONLY_INCLUDE_DIR}")
#   find_package(SDL3 REQUIRED)
#   find_package(SteamworksSDK REQUIRED)
#
#   add_subdirectory(libs/src/glm EXCLUDE_FROM_ALL SYSTEM)
#
#   add_executable(skylabs src/main.cpp)
#   deps_target_link_and_copy_runtime(skylabs PRIVATE
#       SDL3::SDL3
#       SteamworksSDK::SteamAPI
#       glm::glm
#   )
#
# ===============================================================================================
macro(_deps_internal_set_cache_from_env_or_default VAR DEFAULT TYPE DESCRIPTION)
    if(NOT DEFINED ${VAR})
        if(DEFINED ENV{${VAR}})
            set(${VAR} $ENV{${VAR}} CACHE ${TYPE} ${DESCRIPTION})
        else()
            set(${VAR} ${DEFAULT} CACHE ${TYPE} ${DESCRIPTION})
        endif()
    endif()
endmacro()

macro(_deps_internal_set_from_env_or_default VAR DEFAULT)
    if(NOT DEFINED ${VAR})
        if(DEFINED ENV{${VAR}})
            set(${VAR} $ENV{${VAR}})
        else()
            set(${VAR} ${DEFAULT})
        endif()
    endif()
endmacro()

_deps_internal_set_from_env_or_default(DEPS_OUT_SUBDIR "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
_deps_internal_set_from_env_or_default(DEPS_CMAKE_GLOBAL_ARGS "")
_deps_internal_set_from_env_or_default(DEPS_THIRD_PARTY_SUBDIR "third_party")

_deps_internal_set_cache_from_env_or_default(DEPS_SOURCES_DIR "${PROJECT_SOURCE_DIR}/${DEPS_THIRD_PARTY_SUBDIR}/src" PATH "Directory with source libraries")
_deps_internal_set_cache_from_env_or_default(DEPS_INSTALL_DIR "${PROJECT_SOURCE_DIR}/${DEPS_THIRD_PARTY_SUBDIR}/bin/${DEPS_OUT_SUBDIR}" PATH "Directory to install libraries")

_deps_internal_set_cache_from_env_or_default(DEPS_HEADER_SUBDIR "header-only" STRING "Subdirectory name for header-only libraries")
_deps_internal_set_cache_from_env_or_default(DEPS_CACHE_DIR "${DEPS_INSTALL_DIR}/cache" PATH "Directory with git hash")
_deps_internal_set_cache_from_env_or_default(DEPS_PYTHON_PATH "" FILEPATH "Python interpreter executable")
_deps_internal_set_cache_from_env_or_default(DEPS_SCRIPT_PATH "${PROJECT_SOURCE_DIR}/${DEPS_THIRD_PARTY_SUBDIR}/deps.py" FILEPATH "Python helper script path")

mark_as_advanced(DEPS_HEADER_SUBDIR)
mark_as_advanced(DEPS_CACHE_DIR)
mark_as_advanced(DEPS_PYTHON_PATH)
mark_as_advanced(DEPS_SCRIPT_PATH)

set(DEPS_HEADER_ONLY_INCLUDE_DIR "${DEPS_INSTALL_DIR}/${DEPS_HEADER_SUBDIR}")
set(_deps_internal_cmd_args "")

list(APPEND CMAKE_PREFIX_PATH "${DEPS_INSTALL_DIR}")

if(NOT DEPS_PYTHON)
    find_package(Python COMPONENTS Interpreter REQUIRED)
    set(DEPS_PYTHON "${Python_EXECUTABLE}" CACHE FILEPATH "" FORCE)
endif()

# deps_append_cmake_define(VAR_NAME [VALUE])
# Append a -D<VAR_NAME>="<VALUE>" fragment to the DEPS_CMAKE_GLOBAL_ARGS string.
#
# Behavior:
#   - If VALUE is provided, it is used.
#   - Otherwise, if a CMake variable named VAR_NAME is defined, its value is used.
#   - If no value is available, nothing is appended.
#   - Double quotes in the chosen value are escaped.
#
# Example:
#   deps_append_cmake_define(FOO "custom") # -> appends: -DFOO="custom"
#
#   set(FOO "custom")
#   deps_append_cmake_define(FOO) # -> appends: -DFOO="custom"
macro(deps_append_cmake_define VAR)
    if(NOT "${ARGV1}" STREQUAL "")
        string(APPEND DEPS_CMAKE_GLOBAL_ARGS " -D${VAR}=\"${ARGV1}\"")
    else()
        if(DEFINED ${VAR})
            string(APPEND DEPS_CMAKE_GLOBAL_ARGS " -D${VAR}=\"${${VAR}}\"")
        endif()
    endif()
endmacro()

# deps_add_cmake_project(<SOURCE_SUBDIR> [CMAKE_ARGS <args>...] [INSTALL_SUBDIR <dir>] [BUILD_FOLDER <dir>])
#
# Adds a library that is built as a separate CMake project.
#
# Arguments:
#   SOURCE_SUBDIR   - path to the library source directory
#   CMAKE_ARGS      - additional arguments passed to CMake when building the library
#   INSTALL_SUBDIR  - subdirectory for installation (defaults to the name of SOURCE_SUBDIR directory)
#                     (cmake will use: DEPS_INSTALL_DIR/INSTALL_SUBDIR)
#   BUILD_FOLDER    - path to the cmake configure directory (defaults to "build")
#                     (cmake will use: DEPS_SOURCES_DIR/SOURCE_SUBDIR/BUILD_FOLDER)
function(deps_add_cmake_project SOURCE_SUBDIR)
    set(oneValueArgs INSTALL_SUBDIR BUILD_FOLDER)
    set(multiValueArgs CMAKE_ARGS)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    list(JOIN SOURCE_SUBDIR "/" _source_subdir)

    if(ARG_INSTALL_SUBDIR)
        list(JOIN ARG_INSTALL_SUBDIR "/" _install_subdir)
    else()
        get_filename_component(_install_subdir "${_source_subdir}" NAME)
    endif()

    if(ARG_BUILD_FOLDER)
        list(JOIN ARG_BUILD_FOLDER "/" _build_folder)
    else()
        set(_build_folder "build")
    endif()

    if(ARG_CMAKE_ARGS)
        list(JOIN ARG_CMAKE_ARGS " " _cmake_args)
    else()
        set(_cmake_args "")
    endif()

    set(_deps_cmd_args "${_deps_internal_cmd_args}")
    list(APPEND _deps_cmd_args "--cmake-lib" "${_source_subdir}" "${_install_subdir}" "${_build_folder}" "${_cmake_args}")
    set(_deps_internal_cmd_args "${_deps_cmd_args}" PARENT_SCOPE)
endfunction()

# deps_add_header_only(<SOURCE_SUBDIR> [INSTALL_SUBDIR <dir>] [HEADERS <patterns>...])
#
# Registers a header-only library by copying header files from SOURCE_SUBDIR
# to the INSTALL_SUBDIR, matching the given wildcard patterns.
#
# Arguments:
#   SOURCE_SUBDIR   - Source directory containing the headers
#   INSTALL_SUBDIR  - Target subdirectory under the header libs install root (defaults to ".")
#                     (python will copy all files to the DEPS_INSTALL_DIR/DEPS_HEADER_SUBDIR/INSTALL_SUBDIR)
#   HEADERS         - Wildcard patterns of headers to copy
#                     (python will find these files in DEPS_SOURCES_DIR/SOURCE_SUBDIR)
#
# Example:
#   deps_add_header_only(
#       "third_party/my_lib"
#       INSTALL_SUBDIR "include/my_lib"
#       HEADERS "*.h" "internal/*.hpp"
#   )
function(deps_add_header_only SOURCE_SUBDIR)
    set(oneValueArgs INSTALL_SUBDIR)
    set(multiValueArgs HEADERS)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_INSTALL_SUBDIR)
        set(ARG_INSTALL_SUBDIR ".")
    endif()

    set(_source_subdir "${SOURCE_SUBDIR}")
    string(REPLACE ";" " " _source_subdir "${_source_subdir}")

    set(_install_subdir "${ARG_INSTALL_SUBDIR}")
    string(REPLACE ";" " " _install_subdir "${_install_subdir}")

    set(_header_wildcards "${ARG_HEADERS}")

    set(_deps_cmd_args "${_deps_internal_cmd_args}")
    list(APPEND _deps_cmd_args "--header-lib" "${_source_subdir}" "${_install_subdir}" ${_header_wildcards})
    set(_deps_internal_cmd_args "${_deps_cmd_args}" PARENT_SCOPE)
endfunction()

# deps_add_manual_install(<SOURCE_SUBDIR> [INSTALL_SUBDIR <dir>] [RULES <pattern> <dst>...])
#
# Define manual copy/install rules. This option may be repeated.
#
# Arguments:
#   SOURCE_SUBDIR  - Source directory containing files to copy
#   INSTALL_SUBDIR - Target subdirectory under the install root (defaults to SOURCE_SUBDIR directory name)
#                    (python will copy all files to the DEPS_INSTALL_DIR/INSTALL_SUBDIR)
#   RULES          - pairs of: file glob pattern and destination subfolder
#                    (python will find these files in DEPS_SOURCES_DIR/SOURCE_SUBDIR)
#
# Wildcards are supported. When copying directories that include wildcards, a constant
# prefix is ignored.
#
# Example:
# deps_add_manual_install(
#     "SteamworksSDK"
#     INSTALL_SUBDIR "SteamSDK"
#     RULES
#     "redistributable_bin/**/*.dll"      "bin"
# )
#
# `"redistributable_bin/**/*.dll" "bin"`
# copies `DEPS_SOURCES_DIR/SteamworksSDK/redistributable_bin/linux64/libsteam_api.so` into
# `DEPS_INSTALL_DIR/SteamSDK/linux64/bin`.
function(deps_add_manual_install SOURCE_SUBDIR)
    set(oneValueArgs INSTALL_SUBDIR)
    set(multiValueArgs RULES)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_source_subdir "${SOURCE_SUBDIR}")
    string(REPLACE ";" " " _source_subdir "${_source_subdir}")

    if(ARG_INSTALL_SUBDIR)
        list(JOIN ARG_INSTALL_SUBDIR "/" _install_subdir)
    else()
        get_filename_component(_install_subdir "${_source_subdir}" NAME)
    endif()

    set(_pattern_wildcards "${ARG_RULES}")

    set(_deps_cmd_args "${_deps_internal_cmd_args}")
    list(APPEND _deps_cmd_args "--manual-lib" "${_source_subdir}" "${_install_subdir}" ${_pattern_wildcards})
    set(_deps_internal_cmd_args "${_deps_cmd_args}" PARENT_SCOPE)
endfunction()

# deps_build_all()
#
# Builds and installs all registered dependencies using the Python helper script.
#
# Behavior:
#   - Invokes `install_dependencies.py` with collected dependency definitions.
#   - Passes directories, cache, and global CMake arguments as parameters.
#   - Fails the configure step if dependency installation returns a non-zero code.
#
# Example:
#   deps_build_all()
#
# Runs dependency installation for all libraries added via deps_add_* functions.
function(deps_build_all)
    set(DEPS_INSTALL_CMD
        "${DEPS_PYTHON}"
        "${DEPS_SCRIPT_PATH}"
        "--sources-dir" "${DEPS_SOURCES_DIR}"
        "--install-dir" "${DEPS_INSTALL_DIR}"
        "${_deps_internal_cmd_args}"
    )

    if(DEPS_CACHE_DIR)
        list(APPEND DEPS_INSTALL_CMD "--cache-dir=${DEPS_CACHE_DIR}")
    endif()

    if(DEPS_CMAKE_GLOBAL_ARGS)
        list(APPEND DEPS_INSTALL_CMD "--cmake-args=${DEPS_CMAKE_GLOBAL_ARGS}")
    endif()

    if(DEPS_HEADER_SUBDIR)
        list(APPEND DEPS_INSTALL_CMD "--header-subdir=${DEPS_HEADER_SUBDIR}")
    endif()

    cmake_path(GET DEPS_SCRIPT_PATH PARENT_PATH _script_dir)

    execute_process(
        COMMAND ${DEPS_INSTALL_CMD}
        WORKING_DIRECTORY ${_script_dir}
    )
endfunction()

# deps_copy_runtime_binaries(<TARGET> [TARGETS <lib>...])
#
# Copies runtime binaries (DLLs/.so files) of TARGETS next to the TARGET
# after build. On UNIX, also creates symbolic links for .so and .so.0.
#
# Arguments:
#   TARGET   - The main target that depends on the libraries
#   TARGETS  - List of library targets whose runtime files should be copied
#
# Behavior:
#   - Skips INTERFACE libraries.
#   - Copies each runtime binary into $<TARGET_FILE_DIR:${TARGET}>.
#   - On UNIX systems, creates symbolic links:
#       libname.so -> actual file
#       libname.so.0 -> actual file
#
# Example:
# deps_copy_runtime_binaries(MyApp TARGETS MyLib)
#
# Copies MyLib binaries to MyApp's output directory and, on UNIX, creates .so symlinks.
function(deps_copy_runtime_binaries TARGET)
    set(multi_value_args TARGETS)
    cmake_parse_arguments(ARG "" "" "${multi_value_args}" ${ARGN})

    foreach(lib ${ARG_TARGETS})
        if(NOT TARGET ${lib})
            continue()
        endif()

        get_target_property(lib_type ${lib} TYPE)
        if(NOT lib_type OR (NOT(lib_type MATCHES "SHARED_LIBRARY|MODULE_LIBRARY|EXECUTABLE")))
            continue()
        endif()

        set(target_dir $<TARGET_FILE_DIR:${TARGET}>)

        add_custom_command(
            TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${lib}> ${target_dir}
            WORKING_DIRECTORY ${target_dir}
            COMMENT "Copying runtime for ${lib}"
        )

        if(UNIX)
            add_custom_command(
                TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                    $<TARGET_FILE_NAME:${lib}> $<PATH:GET_STEM,$<TARGET_FILE_NAME:${lib}>>.so
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                    $<TARGET_FILE_NAME:${lib}> $<PATH:GET_STEM,$<TARGET_FILE_NAME:${lib}>>.so.0
                WORKING_DIRECTORY ${target_dir}
                COMMENT "Creating symlinks for ${lib}"
            )
        endif()
    endforeach()
endfunction()

# deps_target_link_and_copy_runtime(<target> ... <item>... ...)
# deps_target_link_and_copy_runtime(<target> <PRIVATE|PUBLIC|INTERFACE> <item>... [<PRIVATE|PUBLIC|INTERFACE> <item>...]...)
# deps_target_link_and_copy_runtime(<target> <item>...)
# deps_target_link_and_copy_runtime(<target> <LINK_PRIVATE|LINK_PUBLIC> <lib>... [<LINK_PRIVATE|LINK_PUBLIC> <lib>...]...)
# deps_target_link_and_copy_runtime(<target> LINK_INTERFACE_LIBRARIES <item>...)
#
# Exactly the same as target_link_libraries, but it also calls deps_copy_runtime_binaries
# for all linked libraries.
function(deps_target_link_and_copy_runtime TARGET)
    target_link_libraries(${TARGET} ${ARGN})

    set(linked_libs "")

    foreach(arg IN LISTS ARGN)
        if(
            NOT(arg STREQUAL "PRIVATE") AND
            NOT(arg STREQUAL "PUBLIC") AND
            NOT(arg STREQUAL "INTERFACE") AND
            NOT(arg STREQUAL "LINK_PRIVATE") AND
            NOT(arg STREQUAL "LINK_PUBLIC") AND
            NOT(arg STREQUAL "LINK_INTERFACE_LIBRARIES")
        )
            list(APPEND linked_libs ${arg})
        endif()
    endforeach()

    if(linked_libs)
        deps_copy_runtime_binaries(${TARGET} TARGETS ${linked_libs})
    endif()
endfunction()
