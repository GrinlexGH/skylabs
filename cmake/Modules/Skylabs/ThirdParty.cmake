# Third party libraries
find_package(SteamworksSDK CONFIG)

# Useful libraries
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)

# Use the ENTIRE fucking Vulkan SDK just for shader compiler...
find_package(Vulkan COMPONENTS glslc REQUIRED)
find_package(VulkanHeaders CONFIG REQUIRED)
find_package(VulkanMemoryAllocator-Hpp CONFIG REQUIRED)

# Misc libraries
find_package(Boost COMPONENTS nowide CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(simple_term_colors CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(frozen CONFIG REQUIRED)

find_package(benchmark CONFIG REQUIRED)
