#include <skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp>

namespace Vulkan {
std::size_t DescriptorLayoutHash::operator()(const std::vector<vk::DescriptorSetLayoutBinding>& bindings) const {
    std::size_t seed = 0;
    for (const auto& b : bindings) {
        boost::hash_combine(seed, b.binding);
        boost::hash_combine(seed, static_cast<std::uint32_t>(b.descriptorType));
        boost::hash_combine(seed, b.descriptorCount);
        boost::hash_combine(seed, static_cast<std::uint32_t>(b.stageFlags));
    }
    return seed;
}

CDescriptorLayoutCache::CDescriptorLayoutCache(const CContext& context) : m_device(&*context.Device()) {}

const vk::raii::DescriptorSetLayout& CDescriptorLayoutCache::GetLayout(std::vector<vk::DescriptorSetLayoutBinding> bindings) {
    std::ranges::sort(bindings, [](const auto& a, const auto& b) {
        return a.binding < b.binding;
    });

    auto it = m_cache.find(bindings);
    if (it != m_cache.end()) {
        return it->second;
    }

    vk::DescriptorSetLayoutCreateInfo createInfo {};
    createInfo.setBindings(bindings);

    auto [insertedIt, success] = m_cache.try_emplace(std::move(bindings), vk::raii::DescriptorSetLayout { *m_device, createInfo });

    return insertedIt->second;
}
}
