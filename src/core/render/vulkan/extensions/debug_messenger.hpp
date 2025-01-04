#pragma once

#include "../vulkan.hpp"

bool PrepareDebugUtilsExtension(
    const std::vector<vk::LayerProperties>& availableLayers,
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    std::vector<const char*>& enabledLayers,
    std::vector<const char*>& enabledExtensions,
    vk::DebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCreateInfo
);
