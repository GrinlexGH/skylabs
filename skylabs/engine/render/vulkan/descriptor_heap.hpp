#pragma once
#include "skylabs/engine/render/vulkan/buffer.hpp"

namespace sk::render::vulkan {
class DescriptorHeap {
public:
    explicit DescriptorHeap(const Device& device, const vma::raii::Allocator& allocator);
    DescriptorHeap(const DescriptorHeap&) = delete;
    DescriptorHeap(DescriptorHeap&&) noexcept = default;
    DescriptorHeap& operator=(const DescriptorHeap&) = delete;
    DescriptorHeap& operator=(DescriptorHeap&&) noexcept = default;
    ~DescriptorHeap() = default;

private:
    struct Heap {
        Buffer heap { nullptr };
        vk::DeviceSize reservedOff = 0;
        vk::DeviceSize reservedSize = 0;
    };

    static constexpr std::uint32_t kMaxResourceDescriptors = 64;
    static constexpr std::uint32_t kMaxSamplerDescriptors = 64;

    vk::PhysicalDeviceDescriptorHeapPropertiesEXT m_heapProperties;

    Heap m_resourceHeap;
    Heap m_samplerHeap;

    void CreateResourceHeap();
    void CreateSamplerHeap();
};
}
