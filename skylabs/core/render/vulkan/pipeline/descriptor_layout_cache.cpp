#include <skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp>

namespace Vulkan {
CDescriptorLayoutCache::CDescriptorLayoutCache(const CContext& context) : m_context(&context) {}

const vk::raii::DescriptorSetLayout& CDescriptorLayoutCache::GetLayout(std::vector<vk::DescriptorSetLayoutBinding> bindings) {
    std::ranges::sort(bindings, [](const auto& a, const auto& b) {
        return a.binding < b.binding;
    });

    DescriptorLayoutInfo info { std::move(bindings) };

    auto it = m_cache.find(info);
    if (it != m_cache.end()) {
        return it->second;
    }

    vk::DescriptorSetLayoutCreateInfo createInfo({}, info.bindings);
    auto [insertedIt, success] = m_cache.emplace(std::move(info), vk::raii::DescriptorSetLayout { *m_context->Device(), createInfo });

    return insertedIt->second;
}
}
