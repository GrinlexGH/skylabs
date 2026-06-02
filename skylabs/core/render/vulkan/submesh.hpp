#pragma once
#include <skylabs/core/pch.hpp>

namespace Vulkan {
struct SubMesh {
    std::uint32_t textureIndex = 0;

    std::uint32_t indexCount = 0;
    vma::raii::VirtualAllocation vtxAlloc = nullptr;
    vma::raii::VirtualAllocation idxAlloc = nullptr;

    vk::DeviceSize VtxOffset() const { return vtxAlloc.getInfo().offset; }
    vk::DeviceSize IdxOffset() const { return idxAlloc.getInfo().offset; }
};
}
