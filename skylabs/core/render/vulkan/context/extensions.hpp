#pragma once
#include <vulkan/vulkan.hpp>

namespace Vulkan::Utils {
enum class Requirement : std::uint8_t { eOptional, eRequired };

struct CRequestedExtension
{
    std::string_view m_name;
    Requirement m_requirement;
};

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
}
