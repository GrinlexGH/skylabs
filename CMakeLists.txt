cmake_minimum_required(VERSION 3.27.0)

set(PROJECT_NAME "Skylabs")

project(${PROJECT_NAME} VERSION 0.0.0.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
message(STATUS "Using C standard: c${CMAKE_C_STANDARD}")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "Using C++ standard: c++${CMAKE_CXX_STANDARD}")

add_compile_definitions(UNICODE _UNICODE)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH "x86")
    message("Device architecture: ${ARCH}")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH "x64")
    message("Device architecture: ${ARCH}")
else()
    message(FATAL_ERROR "Unsupported architecture, only x86 and x64 are supported.")
endif()

message("Target architecture: ${CMAKE_GENERATOR_PLATFORM}")

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    if(!MSVC)
        add_compile_options(-g)
    endif()
endif()

set(PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/public)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

