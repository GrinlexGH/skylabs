#pragma once

#include "../vulkan.hpp"

class CInstanceExtensionsCreateInfo;

void AddDebugUtilsLayer(
    const std::vector<vk::LayerProperties>& availableLayers,
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    CInstanceExtensionsCreateInfo& createInfos,
    std::vector<const char*>& enabledLayers,
    std::vector<const char*>& enabledExtensions,
    void*& pNextChain
);
