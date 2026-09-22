#pragma once
#include <deque>

#include <vulkan/vulkan_raii.hpp>

namespace sk::render::vulkan {
class DescriptorWriter {
public:
    explicit DescriptorWriter(const vk::raii::Device& device);
    DescriptorWriter& WriteBuffer(std::uint32_t binding, vk::Buffer buffer, std::size_t size,
                                  std::size_t offset, vk::DescriptorType type,
                                  std::uint32_t arrayElement = 0);
    DescriptorWriter& WriteImage(std::uint32_t binding, vk::ImageView view, vk::Sampler sampler,
                                 vk::ImageLayout layout, vk::DescriptorType type,
                                 std::uint32_t arrayElement = 0);
    void UpdateSet(vk::DescriptorSet set);
    void Clear();

private:
    const vk::raii::Device* m_device = nullptr;

    std::deque<vk::DescriptorImageInfo> m_imageInfos;
    std::deque<vk::DescriptorBufferInfo> m_bufferInfos;
    std::vector<vk::WriteDescriptorSet> m_writes;
};
}
