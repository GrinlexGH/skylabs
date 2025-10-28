cmake_minimum_required(VERSION 3.26)

set(PROJECT_NAME "Skylabs")

project(${PROJECT_NAME} HOMEPAGE_URL "https://grinlexstudios.ru" VERSION 0.0.0 LANGUAGES CXX C)

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
option(SKYLABS_TRY_TO_USE_LIBCXX "Try to use libc++ in skylabs and its dependencies" ON)

# Skylabs directories
set(SKYLABS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SKYLABS_ASSETS_DIR ${SKYLABS_ROOT_DIR}/assets)
set(SKYLABS_ANDROID_DIR ${SKYLABS_ROOT_DIR}/android)
set(SKYLABS_ANDROID_GENERATED_ASSETS_DIR ${SKYLABS_ROOT_DIR}/android/app/src/main/assets)

# Compiler types
set(IS_GNU_LIKE FALSE)
set(IS_MSVC_LIKE FALSE)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(IS_MSVC_LIKE TRUE)
    else()
        set(IS_GNU_LIKE TRUE)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(IS_MSVC_LIKE TRUE)
endif()

# Choosing a runtime
if(IS_GNU_LIKE)
    # GCC implementation by default
    set(STD_RUNTIME "libstdcxx")

    # Use libc++ on linux or msys2 just because I can
    if(SKYLABS_TRY_TO_USE_LIBCXX AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND (UNIX OR (WIN32 AND MINGW)))
        set(STD_RUNTIME "libcxx")
    endif()
else()
    set(STD_RUNTIME "vcruntime")
endif()

# Subdir to separate runtimes
set(DEPS_SUBDIR "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${STD_RUNTIME}/$ENV{DEPS_SUBDIR}")

# Global compilation options
include(Deps)

deps_append_cmake_define(CMAKE_GENERATOR)
deps_append_cmake_define(CMAKE_GENERATOR_PLATFORM)
deps_append_cmake_define(CMAKE_C_COMPILER)
deps_append_cmake_define(CMAKE_CXX_COMPILER)

# Build with libcxx
if(STD_RUNTIME STREQUAL "libcxx")
    if(NOT ANDROID) # Android SDK doesn't support explicitly setting lld
        set(CMAKE_LINKER_TYPE LLD)
    endif()
    add_compile_options("-stdlib=libc++")
    add_link_options("-stdlib=libc++")

    # Compile dependencies with libc++
    deps_append_cmake_define(CMAKE_CXX_FLAGS "-stdlib=libc++")
    deps_append_cmake_define(CMAKE_SHARED_LINKER_FLAGS -stdlib=libc++)
    deps_append_cmake_define(CMAKE_MODULE_LINKER_FLAGS -stdlib=libc++)
    deps_append_cmake_define(CMAKE_EXE_LINKER_FLAGS -stdlib=libc++)
endif()

if(IS_GNU_LIKE)
    add_compile_options(
        "-fdiagnostics-color=always" # I don't wanna read wall of white text on gray background
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-O3>"
    )

    if(NOT ANDROID)
        add_compile_options("-march=native" "-mtune=native")
    endif()

    if(WIN32 AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
        add_compile_options("-fansi-escape-codes")
    endif()
elseif(IS_MSVC_LIKE)
    add_compile_options(
        "/utf-8" # No, I don't want to use one byte encoding in source files
        "/Zc:__cplusplus"
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/Ox>"
    )

    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        add_compile_options("/Gy" "/Gw")
    endif()

    # Clang-cl doesn't support much options
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(
            "/MP"
            "/analyze:external-"
        )

        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
            add_compile_options("/Zc:checkGwOdr")
        endif()
    endif()
endif()

# Enabling LTO
include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result AND NOT(WIN32 AND MINGW))
    message(STATUS "LTO has been enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

if(WIN32)
    add_compile_definitions(UNICODE _UNICODE)
endif()

# Third party shared libraries
deps_append_cmake_define(CMAKE_MSVC_RUNTIME_LIBRARY)

if(ANDROID)
    deps_append_cmake_define(ANDROID_ABI)
    deps_append_cmake_define(CMAKE_ANDROID_ARCH_ABI)
endif()

# Compilation of libraries
deps_add_cmake_project(
    "VulkanMemoryAllocator-Hpp/Vulkan-Headers" INSTALL_SUBDIR "VulkanHeaders"
    CMAKE_ARGS
    "-DVULKAN_HEADERS_ENABLE_TESTS=OFF -DVULKAN_HEADERS_ENABLE_MODULE=ON"
)
deps_add_cmake_project(
    "VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator"
    CMAKE_ARGS
    "-DVMA_BUILD_DOCUMENTATION=OFF -DVMA_BUILD_SAMPLES=OFF -DVMA_ENABLE_INSTALL=ON"
)
deps_add_cmake_project("VulkanMemoryAllocator-Hpp" CMAKE_ARGS "-DVMA_HPP_ENABLE_INSTALL=ON")
deps_add_cmake_project("SDL" INSTALL_SUBDIR "SDL3" CMAKE_ARGS "-DSDL_TEST_LIBRARY=OFF")
deps_add_cmake_project(
    "SDL_image" INSTALL_SUBDIR "SDL3_image"
    CMAKE_ARGS
    "-DSDLIMAGE_AVIF=OFF -DSDLIMAGE_LBM=OFF -DSDLIMAGE_PCX=OFF -DSDLIMAGE_TIF=OFF"
    "-DSDLIMAGE_XCF=OFF -DSDLIMAGE_XPM=OFF -DSDLIMAGE_XV=OFF -DSDLIMAGE_WEBP=OFF"
)

deps_add_header_only("tinyobjloader" HEADERS "tiny_obj_loader.h")
deps_add_header_only("simple_term_colors" HEADERS "include/stc.hpp")

deps_add_manual_install(
    "SteamworksSDK"
    RULES
    "redistributable_bin/**/*.dll"      "bin"
    "public/steam/lib/**/*.dll"         "bin"
    "public/steam/*.h"                  "include/steam"
    "redistributable_bin/**/*.lib"      "lib"
    "redistributable_bin/**/*.so"       "lib"
    "redistributable_bin/**/*.dylib"    "lib"
    "public/steam/lib/**/*.lib"         "lib"
    "public/steam/lib/**/*.so"          "lib"
    "public/steam/lib/**/*.dylib"       "lib"
)

deps_build_all()

# Find these libraries
include_directories(SYSTEM "${DEPS_HEADER_ONLY_INCLUDE_DIR}")

find_package(SteamworksSDK)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(VulkanHeaders REQUIRED)
# Use the ENTIRE fucking Vulkan SDK just for shader compiler...
find_package(Vulkan COMPONENTS glslc REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(VulkanMemoryAllocator-Hpp REQUIRED)

# Third party static libraries
add_subdirectory(third_party/src/nowide EXCLUDE_FROM_ALL SYSTEM)
set(GLM_ENABLE_CXX_20 ON)
add_subdirectory(third_party/src/glm EXCLUDE_FROM_ALL SYSTEM)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(nowide glm PROPERTIES FOLDER "third_party")

# Skylabs
include_directories("${PROJECT_SOURCE_DIR}")
add_subdirectory(skylabs)
