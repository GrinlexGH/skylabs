#pragma once
#include <skylabs/core/pch.hpp>

namespace Vulkan {
class CDescriptorWriter {
public:
    explicit CDescriptorWriter(const vk::raii::Device& device);
    CDescriptorWriter& WriteBuffer(std::uint32_t binding, vk::Buffer buffer, std::size_t size, std::size_t offset, vk::DescriptorType type, std::uint32_t arrayElement = 0);
    CDescriptorWriter& WriteImage(std::uint32_t binding, vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout, vk::DescriptorType type, std::uint32_t arrayElement = 0);
    void UpdateSet(vk::DescriptorSet set);
    void Clear();

private:
    const vk::raii::Device* m_device = nullptr;

    std::deque<vk::DescriptorImageInfo> m_imageInfos;
    std::deque<vk::DescriptorBufferInfo> m_bufferInfos;
    std::vector<vk::WriteDescriptorSet> m_writes;
};
}
