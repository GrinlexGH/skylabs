#include "skylabs/engine/render/vulkan/descriptor_heap.hpp"
#include "skylabs/engine/utils.hpp"

namespace sk::render::vulkan {
DescriptorHeap::DescriptorHeap(const Device& device, const vma::raii::Allocator& allocator) {
    m_heapProperties = device.GetPhysicalDevice()
                           ->getProperties2<vk::PhysicalDeviceProperties2,
                                            vk::PhysicalDeviceDescriptorHeapPropertiesEXT>()
                           .get<vk::PhysicalDeviceDescriptorHeapPropertiesEXT>();

    CreateResourceHeap();
    CreateSamplerHeap();

    const auto sampleHeapUserSize = kMaxSamplerDescriptors * m_heapProperties.samplerDescriptorSize;
    m_samplerHeap.reservedOff =
        utils::AlignUp(sampleHeapUserSize, m_heapProperties.samplerHeapAlignment);
    m_samplerHeap.reservedSize = m_heapProperties.minSamplerHeapReservedRange;
    m_samplerHeap.heap = Buffer { device,
                                  allocator,
                                  m_samplerHeap.reservedOff + m_samplerHeap.reservedSize,
                                  MemoryLocation::eHostVisible,
                                  vk::BufferUsageFlagBits2::eDescriptorHeapEXT,
                                  m_samplerHeap.reservedOff };
}

void DescriptorHeap::CreateResourceHeap() {
    const auto heapUserSize = kMaxResourceDescriptors * m_heapProperties.bufferDescriptorSize;
    m_resourceHeap.reservedOff =
        utils::AlignUp(heapUserSize, m_heapProperties.resourceHeapAlignment);
    m_resourceHeap.reservedSize = m_heapProperties.minResourceHeapReservedRange;
    m_resourceHeap.heap = Buffer { device,
                                   allocator,
                                   m_resourceHeap.reservedOff + m_resourceHeap.reservedSize,
                                   MemoryLocation::eHostVisible,
                                   vk::BufferUsageFlagBits2::eDescriptorHeapEXT,
                                   m_resourceHeap.reservedOff };}

void DescriptorHeap::CreateSamplerHeap() { }
}
