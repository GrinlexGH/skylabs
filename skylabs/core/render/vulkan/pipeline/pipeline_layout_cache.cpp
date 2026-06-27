#include <vulkan/vulkan.h>
#include <skylabs/core/render/vulkan/pipeline/pipeline_layout_cache.hpp>

namespace Vulkan {
bool PipelineLayoutInfo::operator==(const PipelineLayoutInfo& rhs) const {
    return m_descriptorSetLayouts == rhs.m_descriptorSetLayouts && m_pushConstants == rhs.m_pushConstants;
}

std::size_t PipelineLayoutHash::operator()(const PipelineLayoutInfo& info) const {
    std::size_t seed = 0;
    for (const auto& layout : info.m_descriptorSetLayouts) {
        boost::hash_combine(seed, static_cast<VkDescriptorSetLayout>(layout));
    }
    for (const auto& pc : info.m_pushConstants) {
        boost::hash_combine(seed, static_cast<std::uint32_t>(pc.stageFlags));
        boost::hash_combine(seed, pc.offset);
        boost::hash_combine(seed, pc.size);
    }
    return seed;
}

CPipelineLayoutCache::CPipelineLayoutCache(const CContext& context) : m_device(&*context.Device()) {}

const vk::raii::PipelineLayout& CPipelineLayoutCache::GetLayout(PipelineLayoutInfo info) {
    std::ranges::sort(info.m_pushConstants, [](const auto& a, const auto& b) {
        if (a.offset != b.offset) return a.offset < b.offset;
        return a.stageFlags < b.stageFlags;
    });

    auto it = m_cache.find(info);
    if (it != m_cache.end()) {
        return it->second;
    }

    vk::PipelineLayoutCreateInfo createInfo { {}, info.m_descriptorSetLayouts, info.m_pushConstants };

    vk::raii::PipelineLayout layout { *m_device, createInfo };
    auto [insertedIt, success] = m_cache.try_emplace(std::move(info), std::move(layout));

    return insertedIt->second;
}
}
