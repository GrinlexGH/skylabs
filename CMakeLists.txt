set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_minimum_required(VERSION 3.27.0)

set(PROJECT_NAME "Skylabs")

project(${PROJECT_NAME} VERSION 0.0.0.0 LANGUAGES CXX C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
message(STATUS "Using C standard: c${CMAKE_C_STANDARD}")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "Using C++ standard: c++${CMAKE_CXX_STANDARD}")

add_compile_definitions(UNICODE _UNICODE)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH "x86")
	message("Architecture: ${ARCH}")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH "x64")
	message("Architecture: ${ARCH}")
else()
    message(FATAL_ERROR "Unsupported architecture, only x86 and x64 are supported.")
endif()
#######################################
set(SDL_REQUIRED_VER 2.28.1)
set(SDL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2)
message(WARNING "!ATTENSION YOU MUST TO PUT YOUR SDL (include, lib, etc. folders) IN THIS PATH: ${SDL2_PATH}")
set(SDL_LIBRARY ${SDL_PATH}/lib/${ARCH}/SDL2.lib)
set(SDL_INCLUDE_DIR ${SDL_PATH}/include)
#######################################
find_package(SDL ${SDL_REQUIRED_VER})

if(MSVC)
	add_compile_options("/W4" "/WX" "/std:c17")
else()
    add_compile_options("-Wall" "-Wextra" "-Werror")
endif()

message("Target Architecture: ${ARCH}")

set(PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/public)
include_directories(${PUBLIC_INCLUDE_DIR})

foreach(SUBDIR src)
    add_subdirectory("${SUBDIR}")
endforeach()

