#pragma once

#include "../vulkan.hpp"

inline bool HasExtension(const std::vector<vk::ExtensionProperties>& set, const char* target) {
    return std::ranges::any_of(
        set, [&](vk::ExtensionProperties extension) { return strcmp(extension.extensionName, target) == 0; }
    );
}

inline bool HasLayer(const std::vector<vk::LayerProperties>& set, const char* target) {
    return std::ranges::any_of(
        set, [&](vk::LayerProperties extension) { return strcmp(extension.layerName, target) == 0; }
    );
}
