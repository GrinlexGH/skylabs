#pragma once
#include "skylabs/engine/render/vulkan/buffer.hpp"
#include "skylabs/engine/render/vulkan/image.hpp"

namespace sk::render::vulkan {
class DescriptorHeap {
public:
    explicit DescriptorHeap(std::nullptr_t) { }
    explicit DescriptorHeap(const Device& device, const vma::raii::Allocator& allocator);
    DescriptorHeap(const DescriptorHeap&) = delete;
    DescriptorHeap(DescriptorHeap&&) noexcept = default;
    DescriptorHeap& operator=(const DescriptorHeap&) = delete;
    DescriptorHeap& operator=(DescriptorHeap&&) noexcept = default;
    ~DescriptorHeap() = default;

    void BeginFrame(const vk::raii::CommandBuffer& cmd) const;
    void Bind(const vk::raii::CommandBuffer& cmd) const;

    // Naive prototype
    void AllocateAndWriteDescriptors(const std::vector<Buffer>& ubo, const Image& texture);
    void CreateIndexArray();

private:
    struct Heap {
        Buffer heap { nullptr };
        vk::DeviceSize reservedOff = 0;
        vk::DeviceSize reservedSize = 0;
    };

    static constexpr std::uint32_t kMaxResourceDescriptors = 64;
    static constexpr std::uint32_t kMaxSamplerDescriptors = 64;

    const Device* m_device = nullptr;
    const vma::raii::Allocator* m_allocator = nullptr;

    vk::PhysicalDeviceDescriptorHeapPropertiesEXT m_heapProperties;

    Heap m_resourceHeap;
    Heap m_samplerHeap;

    Buffer m_indexArray { nullptr };

    // Naive
    std::vector<vma::raii::VirtualAllocation> m_uboDescriptor;
    vma::raii::VirtualAllocation m_imageDescriptor { nullptr };
    vma::raii::VirtualAllocation m_samplerDescriptor { nullptr };

    void CreateResourceHeap(const Device& device, const vma::raii::Allocator& allocator);
    void CreateSamplerHeap(const Device& device, const vma::raii::Allocator& allocator);
};
}
