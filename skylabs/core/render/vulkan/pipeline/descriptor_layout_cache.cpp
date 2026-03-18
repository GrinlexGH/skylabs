#include <skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp>

#include <boost/container_hash/hash.hpp>

namespace Vulkan {
std::size_t DescriptorLayoutHash::operator()(const DescriptorLayoutInfo& info) const {
    std::size_t seed = 0;
    for (const auto& b : info.m_bindings) {
        boost::hash_combine(seed, b.binding);
        boost::hash_combine(seed, static_cast<std::uint32_t>(b.descriptorType));
        boost::hash_combine(seed, b.descriptorCount);
        boost::hash_combine(seed, static_cast<std::uint32_t>(b.stageFlags));
        boost::hash_combine(seed, b.pImmutableSamplers);
    }
    return seed;
}

CDescriptorLayoutCache::CDescriptorLayoutCache(const CContext& context) : m_context(&context) {}

const vk::raii::DescriptorSetLayout& CDescriptorLayoutCache::GetLayout(DescriptorLayoutInfo info) {
    std::ranges::sort(info.m_bindings, [](const auto& a, const auto& b) {
        return a.binding < b.binding;
    });

    auto it = m_cache.find(info);
    if (it != m_cache.end()) {
        return it->second;
    }

    vk::DescriptorSetLayoutCreateInfo createInfo({}, info.m_bindings);
    vk::raii::DescriptorSetLayout layout { *m_context->Device(), createInfo };
    auto [insertedIt, success] = m_cache.try_emplace(std::move(info), std::move(layout));

    return insertedIt->second;
}
}
