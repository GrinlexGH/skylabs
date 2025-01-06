#pragma once
#include "../vulkan.hpp"
#include "extension_manager.hpp"

void AddDebugUtilsLayer(
    const std::vector<vk::LayerProperties>& availableLayers,
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    CInstanceExtensionsCreateInfos& createInfos,
    std::vector<const char*>& enabledLayers,
    std::vector<const char*>& enabledExtensions,
    void*& pNextChain
);
