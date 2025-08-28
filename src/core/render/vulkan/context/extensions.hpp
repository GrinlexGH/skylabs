#pragma once
#include <vulkan/vulkan.hpp>

inline bool HasExtension(const std::vector<const char*>& set, const std::string_view target) {
    return std::ranges::any_of(
        set, [&](const char* extension) { return extension == target; }
    );
}

inline bool HasExtension(const std::vector<vk::ExtensionProperties>& set, const std::string_view target) {
    return std::ranges::any_of(
        set, [&](const vk::ExtensionProperties& extension) { return extension.extensionName == target; }
    );
}

inline bool HasLayer(const std::vector<const char*>& set, const std::string_view target) {
    return std::ranges::any_of(
        set, [&](const char* layer) { return layer == target; }
    );
}

inline bool HasLayer(const std::vector<vk::LayerProperties>& set, const std::string_view target) {
    return std::ranges::any_of(
        set, [&](const vk::LayerProperties& layer) { return layer.layerName == target; }
    );
}

inline void AppendToPNextChain(void*& currentChain, void* newExtension) {
    if (currentChain == nullptr) {
        currentChain = newExtension;
        return;
    }

    auto* current = static_cast<vk::BaseOutStructure*>(currentChain);
    while (current->pNext != nullptr) {
        current = current->pNext;
    }

    current->pNext = static_cast<vk::BaseOutStructure*>(newExtension);
}
