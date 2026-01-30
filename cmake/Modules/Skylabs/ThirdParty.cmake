# Third party libraries
find_package(SteamworksSDK CONFIG REQUIRED)

# Useful libraries
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)

# Vulkan
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator-Hpp CONFIG REQUIRED)

# Slang target
if(NOT TARGET Vulkan::slangc)
    find_program(Vulkan_SLANGC_EXECUTABLE
        NAMES slangc
        HINTS
            $ENV{VULKAN_SDK}/bin
            $ENV{VULKAN_SDK}/bin32
    )

    if(NOT Vulkan_SLANGC_EXECUTABLE)
        message(FATAL_ERROR "slangc was not found in system!")
    endif()

    add_executable(Vulkan::slangc IMPORTED)
    set_property(TARGET Vulkan::slangc PROPERTY IMPORTED_LOCATION "${Vulkan_SLANGC_EXECUTABLE}")

    unset(Vulkan_SLANGC_EXECUTABLE)
endif()

# Misc libraries
find_package(Boost COMPONENTS nowide CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(simple_term_colors CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(frozen CONFIG REQUIRED)

find_package(benchmark CONFIG REQUIRED)
