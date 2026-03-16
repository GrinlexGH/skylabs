# Third party libraries
find_package(SteamworksSDK CONFIG REQUIRED)

# Useful libraries
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)

# Vulkan
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator-Hpp CONFIG REQUIRED)
find_package(Slangc REQUIRED)

# Misc libraries
find_package(Boost COMPONENTS nowide CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(simple_term_colors CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(frozen CONFIG REQUIRED)

find_package(catch2 CONFIG REQUIRED)
