cmake_minimum_required(VERSION 3.28)
project(Skylabs HOMEPAGE_URL "https://grinlexstudios.ru" VERSION 0.0.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")

# Global settings
include(Skylabs/Skylabs)
include(Skylabs/ThirdParty)

# Skylabs
include_directories("${PROJECT_SOURCE_DIR}")
add_subdirectory(skylabs)
