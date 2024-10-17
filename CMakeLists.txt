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
        "$<$<CONFIG:Release>:-O2>"
        "$<$<CONFIG:RelWithDebInfo>:-O2;-g>"
        "$<$<CONFIG:MinSizeRel>:-Os>"
        "$<$<CONFIG:Debug>:-g;-g3;-Og>"
        "-Wall" "-Wextra" "-Wpedantic" "-Werror"
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
        "/WX" "/W4" "/utf-8" "/Zc:__cplusplus"
    )
else()
    message(FATAL_ERROR "${PROJECT_NAME} Compiles only on gcc, clang, apple clang or msvc!")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/libs/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)
find_package(glm REQUIRED)
find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)
#find_package(GLFW3 REQUIRED)

set(PUBLIC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/public)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
