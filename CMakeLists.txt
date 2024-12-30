cmake_minimum_required(VERSION 3.28)

set(PROJECT_NAME "Skylabs")

project(${PROJECT_NAME} VERSION 0.0.0.0 LANGUAGES CXX C)

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
        "-Wall" "-Wextra" "-Wpedantic" "-Werror"
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
        "/WX" "/W4" "/utf-8" "/Zc:__cplusplus"
    )
else()
    message(WARNING "${PROJECT_NAME} compiles only on gcc, clang, apple clang or msvc! I have no idea if everything is going to be okay")
endif()

set(LIBS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/libs)
set(DLLS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/bin)
set(ROOT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>)

# If you want to compile dependencies with default options, just configure this project.
# If you want to compile dependencies with custom options, run build_dependencies.cmd(.sh)
# with cmake options as arguments and after that configure this project.
# If you want to use your own libraries, then put them into libs/bin/*platform*/*lib name* folder
if(WIN32)
    set(BUILD_SCRIPT_CMD cmd.exe /c ${PROJECT_SOURCE_DIR}/libs/build_dependencies.cmd)
    set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/libs/bin/windows)
    include_directories(${PROJECT_SOURCE_DIR}/libs/bin/windows)
else()
    set(BUILD_SCRIPT_CMD bash ${PROJECT_SOURCE_DIR}/libs/build_dependencies.sh)
    set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/libs/bin/linux)
    include_directories(${PROJECT_SOURCE_DIR}/libs/bin/linux)
endif()

execute_process(
    COMMAND ${BUILD_SCRIPT_CMD}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/")

find_package(glm REQUIRED)
find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(VulkanMemoryAllocator-Hpp REQUIRED)
find_package(tinyobjloader REQUIRED)

add_subdirectory(src)
