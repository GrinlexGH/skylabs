#pragma once

#include "../vulkan.hpp"

inline bool HasExtension(const std::vector<vk::ExtensionProperties>& set, const char* target) {
    return std::ranges::any_of(
        set, [&](vk::ExtensionProperties extension) { return strcmp(extension.extensionName, target) == 0; }
    );
}

inline bool HasExtension(const std::vector<const char*>& set, const char* target) {
    return std::ranges::any_of(
        set, [&](const char* extensionName) { return strcmp(extensionName, target) == 0; });
}

inline bool HasLayer(const std::vector<vk::LayerProperties>& set, const char* target) {
    return std::ranges::any_of(
        set, [&](vk::LayerProperties layer) { return strcmp(layer.layerName, target) == 0; }
    );
}

inline bool HasLayer(const std::vector<const char*>& set, const char* target) {
    return std::ranges::any_of(
        set, [&](const char* layerName) { return strcmp(layerName, target) == 0; });
}

inline void* AppendToPNextChain(void* currentChain, void* newExtension) {
    if (currentChain == nullptr) {
        return newExtension;
    }

    // Go to end of chain
    auto* current = static_cast<vk::BaseOutStructure*>(currentChain);
    while (current->pNext != nullptr) {
        current = current->pNext;
    }

    current->pNext = static_cast<vk::BaseOutStructure*>(newExtension);
    return currentChain;
}

class CInstanceExtensionsCreateInfo
{
public:
    vk::DebugUtilsMessengerCreateInfoEXT m_debugUtilsMessengerCreateInfo;
};
