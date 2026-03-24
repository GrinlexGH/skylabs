#pragma once
#include <skylabs/core/pch.hpp>

namespace Vulkan::Utils {
inline void LinkPNextChain(void*& currentChain, void* newExtension) {
    auto* newStruct = static_cast<vk::BaseOutStructure*>(newExtension);
    newStruct->pNext = static_cast<vk::BaseOutStructure*>(currentChain);
    currentChain = newExtension;
}
}
