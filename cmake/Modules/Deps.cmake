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
#   if(ANDROID)
#       deps_append_cmake_define(ANDROID_ABI)
#       deps_append_cmake_define(CMAKE_ANDROID_ARCH_ABI)
#   endif()
#
#   deps_add_cmake_project("SDL" INSTALL_SUBDIR "SDL3" CMAKE_ARGS -DSDL_TEST_LIBRARY=OFF)
#   deps_add_cmake_project("glm" BUILD_DEBUG)
#   deps_add_header_only("tinyobjloader" HEADERS "tiny_obj_loader.h")
#   deps_add_manual_install(
#       "SteamworksSDK"
#       RULES
#         "redistributable_bin/**/*.dll"      "bin"
#         "public/steam/lib/**/*.dll"         "bin"
#         "public/steam/*.h"                  "include/steam"
#         "redistributable_bin/**/*.lib"      "lib"
#           EXCLUDE "redistributable_bin/**/{libsteam_api.so,libtier0_s.a}"
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
#   find_package(glm REQUIRED)
#
#   add_executable(skylabs src/main.cpp)
#   deps_target_link_libraries(skylabs PRIVATE
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
_deps_internal_set_cache_from_env_or_default(DEPS_CACHE_DIR "" PATH "Directory with git hash (left empty to use library directory)")
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

# deps_add_cmake_project(<SOURCE_SUBDIR> [CMAKE_ARGS <args>...] [INSTALL_SUBDIR <dir>] [BUILD_FOLDER <dir>] [BUILD_DEBUG])
#
# Adds a library that is built as a separate CMake project.
#
# Arguments:
#   SOURCE_SUBDIR   - path to the library source directory
#   CMAKE_ARGS      - additional arguments passed to CMake when building the library
#   INSTALL_SUBDIR  - subdirectory for installation (defaults to the name of SOURCE_SUBDIR directory)
#                     (relative to DEPS_INSTALL_DIR)
#   BUILD_FOLDER    - path to the cmake configure directory (defaults to "build")
#                     (relative to DEPS_SOURCES_DIR/SOURCE_SUBDIR)
#   BUILD_DEBUG     - option whether to also build Debug configuration after Release.
#                     Useful for static libraries (only for windows)
function(deps_add_cmake_project SOURCE_SUBDIR)
    set(options BUILD_DEBUG)
    set(oneValueArgs INSTALL_SUBDIR BUILD_FOLDER)
    set(multiValueArgs CMAKE_ARGS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

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
        set(_cmake_args " ")
    endif()

    set(_deps_cmd_args "${_deps_internal_cmd_args}")
    list(APPEND _deps_cmd_args
        "add-cmake-lib"
        "--src=${_source_subdir}"
        "--install=${_install_subdir}"
        "--build-dir=${_build_folder}"
        "--args=${_cmake_args}"
    )

    if(WIN32 AND ARG_BUILD_DEBUG)
        list(APPEND _deps_cmd_args "--build-debug")
    endif()

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

    set(_header_globs "${ARG_HEADERS}")

    set(_deps_cmd_args "${_deps_internal_cmd_args}")
    list(APPEND _deps_cmd_args 
        "add-header-lib"
        "--src=${_source_subdir}"
        "--install-subdir=${_install_subdir}"
    )

    foreach(wc IN LISTS _header_globs)
        list(APPEND _deps_cmd_args "--glob=${wc}")
    endforeach()

    set(_deps_internal_cmd_args "${_deps_cmd_args}" PARENT_SCOPE)
endfunction()

# deps_add_manual_install(<SOURCE_SUBDIR> [INSTALL_SUBDIR <dir>] [RULES <pattern> <dst> [EXCLUDE <ex>]...])
#
# Define manual copy/install rules. This option may be repeated.
#
# Arguments:
#   SOURCE_SUBDIR  - Source directory containing files to copy
#   INSTALL_SUBDIR - Target subdirectory under the install root (defaults to SOURCE_SUBDIR directory name)
#                    (python will copy all files to the DEPS_INSTALL_DIR/INSTALL_SUBDIR)
#   RULES          - pairs of: file glob pattern and destination subfolder.
#                    You can add EXCLUDE glob right after the rule (exclude glob is
#                    relative to the rule constant prefix before any glob char)
#                    (python will find these files in DEPS_SOURCES_DIR/SOURCE_SUBDIR)
#
# Glob is supported. When copying directories that include wildcards, a constant
# prefix is ignored.
#
# Example:
# deps_add_manual_install(
#     "SteamworksSDK"
#     INSTALL_SUBDIR "SteamSDK"
#     RULES
#       "redistributable_bin/**/*.lib" "lib"
#         EXCLUDE "**/{libsteam_api.so,libtier0_s.a}"
# )
#
# `"redistributable_bin/**/*.lib" "bin"`
# copies `DEPS_SOURCES_DIR/SteamworksSDK/redistributable_bin/linux64/libsteam_api.so` into
# `DEPS_INSTALL_DIR/SteamSDK/linux64/bin`, but ignores any `redistributable_bin/**/{libsteam_api.so,libtier0_s.a}`
# files.
function(deps_add_manual_install SOURCE_SUBDIR)
    set(oneValueArgs INSTALL_SUBDIR)
    set(multiValueArgs RULES)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    list(JOIN SOURCE_SUBDIR "/" _source_subdir)

    if(ARG_INSTALL_SUBDIR)
        list(JOIN ARG_INSTALL_SUBDIR "/" _install_subdir)
    else()
        get_filename_component(_install_subdir "${_source_subdir}" NAME)
    endif()

    set(_deps_cmd_args "${_deps_internal_cmd_args}")
    list(APPEND _deps_cmd_args 
        "add-manual-lib"
        "--src=${_source_subdir}"
        "--install=${_install_subdir}"
    )

    # --- Parser for RULES ---
    # Get the flat list of all arguments that followed the 'RULES' keyword.
    set(rule_items ${ARG_RULES})

    # Initialize a manual index 'i'. We can't use 'foreach' because
    # each rule can consume a variable number of items (2 or 4).
    set(i 0)
    list(LENGTH rule_items num_rule_items)

    # Loop as long as our index is within the bounds of the list.
    while(i LESS num_rule_items)
        # --- Start processing one rule ---

        # 1. Get the mandatory source pattern (e.g., "src/*.h").
        list(GET rule_items ${i} src_pattern)
        
        # 2. Check if a corresponding destination item exists.
        math(EXPR i_plus_1 "${i} + 1")
        if(i_plus_1 GREATER_EQUAL num_rule_items)
            # This is an error: a <src_pattern> was provided without a <dst_subdir>.
            message(FATAL_ERROR "deps_add_manual_install: Missing destination for source pattern '${src_pattern}'")
        endif()

        # 3. Get the mandatory destination sub-directory (e.g., "include").
        list(GET rule_items ${i_plus_1} dst_subdir)

        # 4. Set defaults for this loop iteration.
        # By default, a rule consists of 2 items (<src_pattern> <dst_subdir>).
        set(step 2) 
        # By default, there is no exclude argument.
        set(exclude_arg "") 

        # 5. Check for an optional [EXCLUDE <ex_pattern>] clause.
        # Pre-calculate indices for the 'EXCLUDE' keyword (at i+2) and its pattern (at i+3).
        math(EXPR i_plus_2 "${i} + 2")
        math(EXPR i_plus_3 "${i} + 3")

        # Check if there is at least one more item (at i+2) that *could* be the 'EXCLUDE' keyword.
        if(i_plus_2 LESS num_rule_items)
            # Get the item at i+2 to check if it's the keyword.
            list(GET rule_items ${i_plus_2} maybe_exclude_keyword)

            # If the item is indeed the 'EXCLUDE' keyword...
            if(maybe_exclude_keyword STREQUAL "EXCLUDE")
                # ...then check if the list *also* contains the pattern (at i+3).
                if(i_plus_3 GREATER_EQUAL num_rule_items)
                    # This is an error: found 'EXCLUDE' but no pattern followed it.
                    message(FATAL_ERROR "deps_add_manual_install: EXCLUDE keyword found for '${src_pattern}' but no pattern follows.")
                endif()

                # Get the exclusion pattern string.
                list(GET rule_items ${i_plus_3} ex_pattern)
                # Format the argument for the Python script (e.g., "--ex=src/temp/**").
                set(exclude_arg "--ex=${ex_pattern}")
                # Update the step size to 4, as we consumed 4 items:
                # <src_pattern> <dst_subdir> EXCLUDE <ex_pattern>
                set(step 4)
            endif()
        endif()

        # 6. Append the parsed rule to the main Python command list.
        list(APPEND _deps_cmd_args "rule" "--src=${src_pattern}" "--dst=${dst_subdir}")

        # If the optional exclude argument was set...
        if(NOT "${exclude_arg}" STREQUAL "")
            # ...append it to the command list as well (e.g., "--ex=...").
            list(APPEND _deps_cmd_args "${exclude_arg}")
        endif()

        # 7. Advance the index 'i' by the number of items we just processed (either 2 or 4).
        math(EXPR i "${i} + ${step}")
    endwhile()
    # --- End of parser ---

    set(_deps_internal_cmd_args "${_deps_cmd_args}" PARENT_SCOPE)
endfunction()

# deps_build_all([VERBOSE])
#
# Builds and installs all registered dependencies using the Python helper script.
#
# Arguments:
#   VERBOSE - Show running command line
#
# Behavior:
#   - Invokes Python helper script with collected dependency definitions.
#   - Passes directories, cache, and global CMake arguments as parameters.
#   - Fails the configure step if dependency installation returns a non-zero code.
#
# Example:
#   deps_build_all()
#
# Runs dependency installation for all libraries added via deps_add_* functions.
function(deps_build_all)
    cmake_parse_arguments(ARG "VERBOSE" "" "" ${ARGN})

    set(DEPS_INSTALL_CMD
        "${DEPS_PYTHON}"
        "${DEPS_SCRIPT_PATH}"
        "--sources-dir=${DEPS_SOURCES_DIR}"
        "--install-dir=${DEPS_INSTALL_DIR}"
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

    list(APPEND DEPS_INSTALL_CMD "${_deps_internal_cmd_args}")

    cmake_path(GET DEPS_SCRIPT_PATH PARENT_PATH _script_dir)

    if(ARG_VERBOSE)
        string(JOIN "\n    " _pretty_cmd ${DEPS_INSTALL_CMD})
        message(STATUS "Running command:\n  ${_pretty_cmd}")
    endif()

    execute_process(
        COMMAND ${DEPS_INSTALL_CMD}
        WORKING_DIRECTORY ${_script_dir}
        RESULT_VARIABLE _result
    )

    if(_result)
        message(FATAL_ERROR "Failed to build libraries!")
    endif()
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
            set(symlink_script "${CMAKE_BINARY_DIR}/create_symlink_if_missing.cmake")
            if(NOT EXISTS "${symlink_script}")
                file(WRITE "${symlink_script}"
"if(NOT DEFINED ORIG_FILE)
    message(FATAL_ERROR \"ORIG_FILE not defined\")
endif()

if(NOT DEFINED LINK_NAME)
    message(FATAL_ERROR \"LINK_NAME not defined\")
endif()

get_filename_component(_orig_name \"\${ORIG_FILE}\" NAME)
get_filename_component(_link_name \"\${LINK_NAME}\" NAME)

if(NOT (_orig_name STREQUAL _link_name))
    file(CREATE_LINK \"\${ORIG_FILE}\" \"\${LINK_NAME}\" SYMBOLIC)
endif()
"
                )
            endif()

            set(stem_expr "$<PATH:GET_STEM,$<TARGET_FILE_NAME:${lib}>>")

            add_custom_command(
                TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -DORIG_FILE="$<TARGET_FILE_NAME:${lib}>" -DLINK_NAME="${stem_expr}.so" -P "${symlink_script}"
                COMMAND ${CMAKE_COMMAND} -DORIG_FILE="$<TARGET_FILE_NAME:${lib}>" -DLINK_NAME="${stem_expr}.so.0" -P "${symlink_script}"
                WORKING_DIRECTORY ${target_dir}
                COMMENT "Creating symlinks for ${lib}"
            )
        endif()
    endforeach()
endfunction()

function(_deps_internal_add_processed_target tgt)
    get_property(_processed_targets GLOBAL PROPERTY _deps_internal_processed_targets)
    if(NOT _processed_targets)
        set(_processed_targets "")
    endif()

    if(NOT ("${tgt}" IN_LIST _processed_targets))
        list(APPEND _processed_targets "${tgt}")
        set_property(GLOBAL PROPERTY _deps_internal_processed_targets "${_processed_targets}")
    endif()
endfunction()

function(_deps_internal_is_processed_target tgt out_var)
    get_property(_processed_targets GLOBAL PROPERTY _deps_internal_processed_targets)
    if(NOT _processed_targets)
        set(${out_var} FALSE PARENT_SCOPE)
        return()
    endif()

    if("${tgt}" IN_LIST _processed_targets)
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Recursively go through all the dependencies for imported targets
function(_deps_internal_map_configs target)
    # Handle LINK_ONLY libraries
    if("${target}" MATCHES "\\$<LINK_ONLY:([^>]+)>")
        string(REGEX REPLACE "\\$<LINK_ONLY:([^>]+)>" "\\1" target "${target}")
    endif()

    if(NOT TARGET ${target})
        return()
    endif()

    # Cache
    _deps_internal_is_processed_target("${target}" already)
    if(already)
        return()
    endif()
    _deps_internal_add_processed_target("${target}")

    get_target_property(target_type ${target} TYPE)
    get_target_property(is_imported ${target} IMPORTED)
    if(is_imported AND (target_type MATCHES "STATIC_LIBRARY|OBJECT_LIBRARY|INTERFACE_LIBRARY"))
        # Resolve alias
        get_target_property(real ${target} ALIASED_TARGET)
        if(real)
            set(target ${real})
        endif()

        # Change to release
        set_target_properties(
            ${target} PROPERTIES
            MAP_IMPORTED_CONFIG_MINSIZEREL Release
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        )

        # Ignore missing pdbs
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            target_link_options(${target} INTERFACE "/ignore:4099")
        endif()
    endif()

    # Handle dependencies
    get_target_property(libs ${target} INTERFACE_LINK_LIBRARIES)
    if(NOT libs)
        return()
    endif()

    foreach(lib IN LISTS libs)
        _deps_internal_map_configs(${lib})
    endforeach()
endfunction()

# deps_target_link_libraries(<target> [<visibility>] <item>... [<visibility> <item>...]...)
#
# Wrapper for `target_link_libraries` that also copies runtime binaries
# (DLLs / shared libraries) of all linked targets.
#
# This behaves the same as `target_link_libraries`, but after linking,
# it calls `deps_copy_runtime_binaries()` to ensure dependent shared libraries
# are copied next to the target.
#
# Additionally, STATIC, INTERFACE and OBJECT library targets are patched with
# MAP_IMPORTED_CONFIG_RELWITHDEBINFO/MINSIZEREL=Release to fix configuration
# mismatches.
#
# Usage:
#   deps_target_link_libraries(MyApp PRIVATE FooLib BarLib)
#   deps_target_link_libraries(MyApp PUBLIC SDL2::SDL2)
function(deps_target_link_libraries TARGET)
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

    # Hack for static libraries
    foreach(target IN LISTS linked_libs)
        _deps_internal_map_configs(${target})
    endforeach()

    target_link_libraries(${TARGET} ${ARGN})

    deps_copy_runtime_binaries(${TARGET} TARGETS ${linked_libs})
endfunction()
