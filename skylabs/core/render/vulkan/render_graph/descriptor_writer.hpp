#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <deque>

namespace Vulkan {
class CDescriptorWriter {
public:
    CDescriptorWriter& WriteBuffer(uint32_t binding, vk::Buffer buffer, size_t size, size_t offset, vk::DescriptorType type, uint32_t arrayElement = 0);
    CDescriptorWriter& WriteImage(uint32_t binding, vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout, vk::DescriptorType type, uint32_t arrayElement = 0);
    void UpdateSet(vk::Device device, vk::DescriptorSet set);
    void Clear();

private:
    std::deque<vk::DescriptorImageInfo> m_imageInfos;
    std::deque<vk::DescriptorBufferInfo> m_bufferInfos;
    std::vector<vk::WriteDescriptorSet> m_writes;
};
}
