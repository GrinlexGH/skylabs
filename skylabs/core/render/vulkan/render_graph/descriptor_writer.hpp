#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <deque>

namespace Vulkan {
class CDescriptorWriter {
public:
    CDescriptorWriter(const CContext& context);
    CDescriptorWriter& WriteBuffer(uint32_t binding, vk::Buffer buffer, size_t size, size_t offset, vk::DescriptorType type, uint32_t arrayElement = 0);
    CDescriptorWriter& WriteImage(uint32_t binding, vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout, vk::DescriptorType type, uint32_t arrayElement = 0);
    void UpdateSet(vk::DescriptorSet set);
    void Clear();

private:
    const vk::raii::Device* m_device = nullptr;

    std::deque<vk::DescriptorImageInfo> m_imageInfos;
    std::deque<vk::DescriptorBufferInfo> m_bufferInfos;
    std::vector<vk::WriteDescriptorSet> m_writes;
};
}
