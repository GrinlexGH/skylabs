#pragma once

#include "../vulkan.hpp"

bool PrepareDebugUtilsExtension(
    vk::DebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCreateInfo,
    std::vector<const char*>& enabledExtensions,
    std::vector<const char*>& enabledLayers,
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    const std::vector<vk::LayerProperties>& availableLayers
);
