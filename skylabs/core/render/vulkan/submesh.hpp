#pragma once
#include <vk_mem_alloc_raii.hpp>

namespace sk::render::vulkan {
struct SubMesh {
    std::uint32_t indexCount = 0;
    vma::raii::VirtualAllocation vtxAlloc = nullptr;
    vma::raii::VirtualAllocation idxAlloc = nullptr;

    vk::DeviceSize VtxOffset() const { return vtxAlloc.getInfo().offset; }
    vk::DeviceSize IdxOffset() const { return idxAlloc.getInfo().offset; }
};
}
