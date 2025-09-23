cmake_minimum_required(VERSION 3.26)

set(PROJECT_NAME "Skylabs")

project(${PROJECT_NAME} HOMEPAGE_URL "https://grinlexstudios.ru" VERSION 0.0.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

# Options and defaults
if(ANDROID)
    set(DEFAULT_ENABLE_STEAM OFF)
else()
    set(DEFAULT_ENABLE_STEAM ON)
endif()

option(ENABLE_STEAM "Enable steamapi" ${DEFAULT_ENABLE_STEAM})

# Skylabs directories
set(SKYLABS_ROOT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(SKYLABS_ASSETS_DIRECTORY ${SKYLABS_ROOT_DIRECTORY}/assets)
set(SKYLABS_ANDROID_DIRECTORY ${SKYLABS_ROOT_DIRECTORY}/android)
set(SKYLABS_ANDROID_GENERATED_ASSETS_DIRECTORY ${SKYLABS_ROOT_DIRECTORY}/android/app/src/main/assets)

# Build and install third party shared libraries
macro(set_from_env_or_default VAR DEFAULT)
    if(NOT ${VAR})
        if(DEFINED ENV{${VAR}})
            set(${VAR} $ENV{${VAR}})
        else()
            set(${VAR} ${DEFAULT})
        endif()
    endif()
endmacro()

set_from_env_or_default(DEPS_TARGET_SYSTEM ${CMAKE_SYSTEM_NAME})
set_from_env_or_default(DEPS_TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
set_from_env_or_default(DEPS_SUBFOLDER "")

set(DEPS_INSTALL_DIR "${PROJECT_SOURCE_DIR}/libs/bin/${DEPS_TARGET_SYSTEM}-${DEPS_TARGET_ARCH}/${DEPS_SUBFOLDER}")

set(DEPS_CMAKE_ARGS CACHE STRING "Global cmake arguments for all libraries.")
set(DEPS_CMAKE_LIB_ARGS CACHE STRING "Library-specific cmake arguments. Use format: name=(args). Example: zlib=(-DSKIP_EXAMPLES=ON) SDL=(-DOPTION=VALUE)")

find_package(Python COMPONENTS Interpreter)

set(DEPS_INSTALL_CMD
    ${Python_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/libs/install_dependencies.py
    "--install-dir"
    "${DEPS_INSTALL_DIR}"
)

macro(set_env_if_unset VAR)
    if(NOT ${VAR} AND DEFINED ENV{${VAR}})
        set(${VAR} $ENV{${VAR}})
    endif()
endmacro()

set_env_if_unset(DEPS_CMAKE_ARGS)
set_env_if_unset(DEPS_CMAKE_LIB_ARGS)

# Manually set android abi
macro(append_deps_cmake_args_if_defined VAR)
    if(DEFINED ${VAR})
        string(APPEND DEPS_CMAKE_ARGS " -D${VAR}=\"${${VAR}}\"")
    endif()
endmacro()

if(ANDROID)
    append_deps_cmake_args_if_defined(ANDROID_ABI)
    append_deps_cmake_args_if_defined(CMAKE_ANDROID_ARCH_ABI)
endif()

if(DEPS_CMAKE_ARGS)
    list(APPEND DEPS_INSTALL_CMD "--cmake-args=${DEPS_CMAKE_ARGS}")
endif()

if(DEPS_CMAKE_LIB_ARGS)
    list(APPEND DEPS_INSTALL_CMD "--cmake-lib-args=${DEPS_CMAKE_LIB_ARGS}")
endif()

execute_process(
    COMMAND ${DEPS_INSTALL_CMD}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libs
    RESULT_VARIABLE ret
)

if(NOT(ret EQUAL 0))
    message(FATAL_ERROR "Error when building or installing dependencies!")
endif()

# Find these libraries
include_directories(SYSTEM "${DEPS_INSTALL_DIR}/header-only")
list(APPEND CMAKE_PREFIX_PATH "${DEPS_INSTALL_DIR}")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")

find_package(SteamworksSDK)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(VulkanHeaders REQUIRED)
# Use the ENTIRE fucking Vulkan SDK just for shader compiler...
find_package(Vulkan COMPONENTS glslc REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(VulkanMemoryAllocator-Hpp REQUIRED)

# Global options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
    add_compile_options(
        "-fdiagnostics-color=always" # I don't wanna read wall of white text on gray background
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-O3>"
    )

    # Android SDK doesn't support march native. Kind of logical
    if(NOT ANDROID)
        add_compile_options(
            "-march=native" "-mtune=native"
        )
    endif()

    # Use libc++ on linux or msys2 just because I can
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND ((WIN32 AND MINGW) OR NOT CMAKE_CXX_SIMULATE_ID MATCHES "MSVC"))
        if(NOT ANDROID) # Android SDK doesn't support explicitly setting lld
            set(CMAKE_LINKER_TYPE LLD)
        endif()
        add_compile_options("-stdlib=libc++")
        add_link_options("-stdlib=libc++")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
        "/utf-8" # No, I don't want to use one byte encoding
        "/Zc:__cplusplus" # Idk what it is fixing, but it is definitely fixing something
        "/MP" # Multithreading
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/Ox>"
    )
endif()

# Enabling LTO
include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result)
    if(WIN32 AND NOT MINGW)
        message(STATUS "LTO has been enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endif()

if(WIN32)
    add_compile_definitions(UNICODE _UNICODE)
endif()

# Third party static libraries
add_subdirectory(libs/sources/Boost.Nowide EXCLUDE_FROM_ALL SYSTEM)
set(GLM_ENABLE_CXX_20 ON)
add_subdirectory(libs/sources/glm EXCLUDE_FROM_ALL SYSTEM)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(nowide glm PROPERTIES FOLDER "third_party")

# Skylabs
add_subdirectory(src)
