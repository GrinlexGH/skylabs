cmake_minimum_required(VERSION 3.25)

set(PROJECT_NAME "Skylabs")

project(${PROJECT_NAME} HOMEPAGE_URL "https://grinlexstudios.ru" VERSION 0.0.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

# Standard library options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang" AND NOT CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
    set(CMAKE_LINKER_TYPE LLD)
    add_compile_options("-stdlib=libc++")
    add_link_options("-stdlib=libc++")
endif()

# Third party static libraries
add_subdirectory(libs/sources/Boost.Nowide EXCLUDE_FROM_ALL SYSTEM)
set(GLM_ENABLE_CXX_20 ON)
add_subdirectory(libs/sources/glm EXCLUDE_FROM_ALL SYSTEM)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(nowide glm PROPERTIES FOLDER "third_party")

# Build and install third party shared libraries
if(NOT DEPS_TARGET_SYSTEM)
    if(DEFINED ENV{DEPS_TARGET_SYSTEM})
        set(DEPS_TARGET_SYSTEM $ENV{DEPS_TARGET_SYSTEM})
    else()
        set(DEPS_TARGET_SYSTEM ${CMAKE_SYSTEM_NAME})
    endif()
endif()

if(NOT DEPS_TARGET_ARCH)
    if(DEFINED ENV{DEPS_TARGET_ARCH})
        set(DEPS_TARGET_ARCH $ENV{DEPS_TARGET_ARCH})
    else()
        set(DEPS_TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
    endif()
endif()

if(NOT DEPS_SUBFOLDER)
    if(DEFINED ENV{DEPS_SUBFOLDER})
        set(DEPS_SUBFOLDER $ENV{DEPS_SUBFOLDER})
    else()
        set(DEPS_SUBFOLDER "")
    endif()
endif()

set(DEPS_INSTALL_DIR "${PROJECT_SOURCE_DIR}/libs/bin/${DEPS_TARGET_SYSTEM}-${DEPS_TARGET_ARCH}/${DEPS_SUBFOLDER}")

set(DEPS_CMAKE_ARGS CACHE STRING "Global cmake arguments for all libraries.")
set(DEPS_CMAKE_LIB_ARGS CACHE STRING "Library-specific cmake arguments. Use format: name=(args). Example: zlib=(-DSKIP_EXAMPLES=ON) SDL=(-DOPTION=VALUE)")

set(DEPS_INSTALL_CMD
    python
    ${PROJECT_SOURCE_DIR}/libs/install_dependencies.py
    "--install-dir"
    "${DEPS_INSTALL_DIR}"
)

if(NOT DEPS_CMAKE_ARGS AND DEFINED ENV{DEPS_CMAKE_ARGS})
    set(DEPS_CMAKE_ARGS $ENV{DEPS_CMAKE_ARGS})
endif()

if(DEPS_CMAKE_ARGS)
    list(APPEND DEPS_INSTALL_CMD "--cmake-args=${DEPS_CMAKE_ARGS}")
endif()

if(NOT DEPS_CMAKE_LIB_ARGS AND DEFINED ENV{DEPS_CMAKE_LIB_ARGS})
    set(DEPS_CMAKE_LIB_ARGS $ENV{DEPS_CMAKE_LIB_ARGS})
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

find_package(SteamworksSDK REQUIRED)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(VulkanMemoryAllocator-Hpp REQUIRED)

# Skylabs
add_subdirectory(src)
