cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "Skylabs")

project(${PROJECT_NAME} HOMEPAGE_URL "https://grinlexstudios.ru" VERSION 0.0.0.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
message(STATUS "Using C standard: c${CMAKE_C_STANDARD}")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "Using C++ standard: c++${CMAKE_CXX_STANDARD}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(
        "-Wall" "-Wextra" "-Wpedantic"
        "$<$<CONFIG:Release>:-Werror>"
        "-march=native" "-mtune=native"
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(COMPILER_MSVC ON)
    add_link_options("/ignore:4099")
    add_compile_options(
        "/W4" "/utf-8" "/Zc:__cplusplus"
        "$<$<CONFIG:Release>:/WX>" "/MP"
    )
else()
    message(WARNING "${PROJECT_NAME} compiles only on gcc, clang, apple clang or msvc! I have no idea if everything is going to be okay")
endif()

set(ROOT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/)
set(DLLS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/bin)
set(LIBS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/libs)
set(SHADER_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/shaders)

if(WIN32)
    if(COMPILER_MSVC)
        set(DEPENDENCIES_COMPILER "msvc")
    else()
        set(DEPENDENCIES_COMPILER "mingw")
    endif()

    set(BUILD_SCRIPT_CMD pwsh -ExecutionPolicy Bypass -File ${PROJECT_SOURCE_DIR}/libs/build_dependencies.ps1 ${DEPENDENCIES_COMPILER})
    set(CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/libs/bin/windows;${PROJECT_SOURCE_DIR}/libs/bin/windows/static/${DEPENDENCIES_COMPILER}")
else()
    set(BUILD_SCRIPT_CMD bash ${PROJECT_SOURCE_DIR}/libs/build_dependencies.sh)
    set(CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/libs/bin/unix;${PROJECT_SOURCE_DIR}/libs/bin/unix/static/")
endif()

execute_process(
    COMMAND ${BUILD_SCRIPT_CMD}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/")

find_package(nowide REQUIRED)
find_package(glm REQUIRED)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(VulkanMemoryAllocator-Hpp REQUIRED)
find_package(tinyobjloader REQUIRED)

add_subdirectory(src)
