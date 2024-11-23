cmake_minimum_required(VERSION 3.28)

set(PROJECT_NAME "Skylabs")

project(${PROJECT_NAME} VERSION 0.0.0.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
message(STATUS "Using C standard: c${CMAKE_C_STANDARD}")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "Using C++ standard: c++${CMAKE_CXX_STANDARD}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(
        "-Wall" "-Wextra" "-Wpedantic" "-Werror"
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
        "/WX" "/W4" "/utf-8" "/Zc:__cplusplus"
    )
else()
    message(FATAL_ERROR "${PROJECT_NAME} Compiles only on gcc, clang, apple clang or msvc!")
endif()

# TODO: options for building dependencies
if(MSVC)
    if(CMAKE_GENERATOR_PLATFORM)
        set(CMAKE_OPTIONS
            -A ${CMAKE_GENERATOR_PLATFORM}
        )
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
        set(CMAKE_OPTIONS
            ${CMAKE_OPTIONS}
            -T ${CMAKE_GENERATOR_TOOLSET}
        )
    endif()
endif()

option(SKYLABS_BUILD_DEPENDENCIES "Should you build all dependencies from the libs folder." ON)

if (SKYLABS_BUILD_DEPENDENCIES)
    if(WIN32)
        set(BUILD_SCRIPT_CMD cmd.exe /c ${PROJECT_SOURCE_DIR}/libs/build_dependencies.cmd)
    else()
        set(BUILD_SCRIPT_CMD bash ${PROJECT_SOURCE_DIR}/libs/build_dependencies.sh)
    endif()

    execute_process(
        COMMAND ${BUILD_SCRIPT_CMD}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/
    )

    list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/")

    if(WIN32)
        set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/libs/bin/windows)
    elseif(UNIX)
        set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/libs/bin/linux)
    endif()
endif()

find_package(glm REQUIRED)
find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(VulkanMemoryAllocator-Hpp REQUIRED)
find_package(tinyobjloader REQUIRED)
#find_package(GLFW3 REQUIRED)

set(PUBLIC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/public)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
