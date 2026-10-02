#include "skylabs/engine/render/vulkan/pipeline_layout_cache.hpp"
#include "skylabs/engine/utils.hpp"

namespace sk::render::vulkan {
bool PipelineLayoutInfo::operator==(const PipelineLayoutInfo& rhs) const {
    return descriptorSetLayouts == rhs.descriptorSetLayouts && pushConstants == rhs.pushConstants;
}

std::size_t PipelineLayoutHash::operator()(const PipelineLayoutInfo& info) const {
    std::size_t seed = 0;
    for (const auto& layout : info.descriptorSetLayouts) {
        utils::HashCombine(seed, static_cast<VkDescriptorSetLayout>(layout));
    }

    for (const auto& pc : info.pushConstants) {
        utils::HashCombine(seed, static_cast<std::uint32_t>(pc.stageFlags));
        utils::HashCombine(seed, pc.offset);
        utils::HashCombine(seed, pc.size);
    }

    return seed;
}

PipelineLayoutCache::PipelineLayoutCache(const vk::raii::Device& device) : m_device(&device) { }

const vk::raii::PipelineLayout& PipelineLayoutCache::GetLayout(PipelineLayoutInfo layoutInfo) {
    std::ranges::sort(layoutInfo.pushConstants, [](const auto& a, const auto& b) {
        if (a.offset != b.offset) return a.offset < b.offset;
        return a.stageFlags < b.stageFlags;
    });

    if (const auto it = m_cache.find(layoutInfo); it != m_cache.end()) {
        return it->second;
    }

    const vk::PipelineLayoutCreateInfo createInfo { { },
                                                    layoutInfo.descriptorSetLayouts,
                                                    layoutInfo.pushConstants };

    vk::raii::PipelineLayout layout { *m_device, createInfo };
    auto [insertedIt, success] = m_cache.try_emplace(std::move(layoutInfo), std::move(layout));

    return insertedIt->second;
}
}
