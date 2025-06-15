cmake_minimum_required(VERSION 3.25)

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

add_subdirectory(libs/sources/Boost.Nowide EXCLUDE_FROM_ALL SYSTEM)
set(GLM_ENABLE_CXX_20 ON)
add_subdirectory(libs/sources/glm EXCLUDE_FROM_ALL SYSTEM)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(nowide glm PROPERTIES FOLDER "third_party")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(
        "-Wall" "-Wextra" "-Wpedantic"
        "$<$<CONFIG:Release>:-Werror>"
        "-march=native" "-mtune=native"
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
        "/W4" "/utf-8" "/Zc:__cplusplus" "/MP"
        "$<$<CONFIG:Release>:/WX>"
    )
else()
    message(WARNING "${PROJECT_NAME} compiles only on gcc, clang, apple clang or msvc! I have no idea if everything is going to be okay")
endif()

set(ROOT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/)
set(DLLS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/bin)
set(LIBS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/libs)
set(SHADER_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/shaders)

set(DEPS_INSTALL_DIR "${PROJECT_SOURCE_DIR}/libs/bin/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

set(DEPS_CMAKE_ARGS CACHE STRING "Global cmake arguments for all libraries.")
set(DEPS_CMAKE_LIB_ARGS CACHE STRING "Library-specific cmake arguments. Use format: name=(args). Example: zlib=(-DSKIP_EXAMPLES=ON) SDL=(-DOPTION=VALUE)")

set(DEPS_INSTALL_CMD
    python
    ${PROJECT_SOURCE_DIR}/libs/install_dependencies.py
    "--install-dir"
    "${DEPS_INSTALL_DIR}"
    "--cmake-sizeof-void-p"
    "${CMAKE_SIZEOF_VOID_P}"
)

if(DEPS_CMAKE_ARGS)
    list(APPEND DEPS_INSTALL_CMD "--cmake-args" "${DEPS_CMAKE_ARGS}")
endif()

if(DEPS_CMAKE_LIB_ARGS)
    list(APPEND DEPS_INSTALL_CMD "--cmake-lib-args" "${DEPS_CMAKE_LIB_ARGS}")
endif()

execute_process(
    COMMAND ${DEPS_INSTALL_CMD}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libs
    RESULT_VARIABLE ret
)

if(NOT(ret EQUAL 0))
    message(FATAL_ERROR "Error when building or installing dependencies!")
endif()

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

add_subdirectory(src)
