# Third party shared libraries
include(Deps)

deps_append_cmake_define(CMAKE_MSVC_RUNTIME_LIBRARY)

if(ANDROID)
    deps_append_cmake_define(ANDROID_ABI)
    deps_append_cmake_define(CMAKE_ANDROID_ARCH_ABI)
endif()

# Compilation of libraries
deps_add_cmake_project(
    "VulkanMemoryAllocator-Hpp/Vulkan-Headers" INSTALL_SUBDIR "VulkanHeaders"
    CMAKE_ARGS
    "-DVULKAN_HEADERS_ENABLE_TESTS=OFF -DVULKAN_HEADERS_ENABLE_INSTALL=ON"
)
deps_add_cmake_project(
    "VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator"
    CMAKE_ARGS
    "-DVMA_BUILD_DOCUMENTATION=OFF -DVMA_BUILD_SAMPLES=OFF -DVMA_ENABLE_INSTALL=ON"
)
deps_add_cmake_project(
    "VulkanMemoryAllocator-Hpp"
    CMAKE_ARGS
    "-DVMA_HPP_ENABLE_INSTALL=ON -DVMA_BUILD_CXX_MODULE=OFF"
)
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
find_package(Vulkan COMPONENTS glslc REQUIRED) # Use the ENTIRE fucking Vulkan SDK just for shader compiler...
find_package(VulkanMemoryAllocator REQUIRED)
find_package(VulkanMemoryAllocator-Hpp REQUIRED)

# Third party static libraries
add_subdirectory(third_party/src/nowide EXCLUDE_FROM_ALL SYSTEM)
set(GLM_ENABLE_CXX_20 ON)
add_subdirectory(third_party/src/glm EXCLUDE_FROM_ALL SYSTEM)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(nowide glm PROPERTIES FOLDER "third_party")
