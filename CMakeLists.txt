cmake_minimum_required(VERSION 3.28)
project(Skylabs HOMEPAGE_URL "https://grinlexstudios.ru" VERSION 0.0.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")

# Options
if(ANDROID)
    set(enable_steam_default OFF)
else()
    set(enable_steam_default ON)
endif()

option(SKYLABS_ENABLE_STEAM "Enable Steamworks SDK" ${enable_steam_default})

# Global settings
include(Skylabs/Skylabs)
include(Skylabs/CompileOptions)
include(Skylabs/ThirdParty)

# Skylabs directories
set(SKYLABS_ROOT_DIR ${PROJECT_SOURCE_DIR})
set(SKYLABS_ASSETS_DIR ${SKYLABS_ROOT_DIR}/assets)
set(SKYLABS_ANDROID_DIR ${SKYLABS_ROOT_DIR}/android)
set(SKYLABS_ANDROID_GENERATED_ASSETS_DIR ${SKYLABS_ROOT_DIR}/android/app/src/main/assets)

# Skylabs
include_directories("${PROJECT_SOURCE_DIR}")
add_subdirectory(skylabs)
