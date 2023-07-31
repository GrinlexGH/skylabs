set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_minimum_required(VERSION 3.27.0)

set(PROJECT_NAME "Skylabs")

project(${PROJECT_NAME} VERSION 0.0.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
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
set(SDL2_REQUIRED_VER 2.28.1)
set(SDL2_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2)
message(WARNING "!ATTENSION YOU MUST TO PUT YOUR SDL (include, lib, etc. folders) IN THIS PATH: ${SDL2_PATH}")
set(SDL2_LIBRARY ${SDL2_PATH}/lib/${ARCH}/SDL2.lib)
set(SDL2MAIN_LIBRARY "${SDL2_PATH}/lib/${ARCH}/SDL2main.lib")
set(SDL2_INCLUDE_DIR ${SDL2_PATH}/include)
#######################################
find_package(SDL2 ${SDL2_REQUIRED_VER})

if(MSVC)
	add_compile_options("/W4" "/WX")
else()
    add_compile_options("-Wall" "-Wextra" "-Werror")
endif()

set(PUBLIC_FOLDERS
	tier0
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TARGET_ARCHITECTURE "x64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(TARGET_ARCHITECTURE "x32")
else()
	message(FATAL_ERROR "UNSUPPORTED ARCHITECTURE")
endif()

message("Target Architecture: ${TARGET_ARCHITECTURE}")

set(PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/public)
include_directories(${PUBLIC_INCLUDE_DIR})

foreach(SUBDIR src)
    add_subdirectory("${SUBDIR}")
endforeach()

