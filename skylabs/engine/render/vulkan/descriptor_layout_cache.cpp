#include "skylabs/engine/render/vulkan/descriptor_layout_cache.hpp"
#include "skylabs/engine/utils.hpp"

namespace sk::render::vulkan {
std::size_t DescriptorLayoutHash::operator()(
    const std::vector<vk::DescriptorSetLayoutBinding>& bindings) const {
    std::size_t seed = 0;
    for (const auto& b : bindings) {
        utils::HashCombine(seed, b.binding);
        utils::HashCombine(seed, static_cast<std::uint32_t>(b.descriptorType));
        utils::HashCombine(seed, b.descriptorCount);
        utils::HashCombine(seed, static_cast<std::uint32_t>(b.stageFlags));
    }
    return seed;
}

DescriptorLayoutCache::DescriptorLayoutCache(const vk::raii::Device& device) : m_device(&device) { }

const vk::raii::DescriptorSetLayout& DescriptorLayoutCache::GetLayout(
    std::vector<vk::DescriptorSetLayoutBinding> bindings) {
    std::ranges::sort(bindings, [](const auto& a, const auto& b) { return a.binding < b.binding; });

    if (const auto it = m_cache.find(bindings); it != m_cache.end()) {
        return it->second;
    }

    vk::DescriptorSetLayoutCreateInfo createInfo { };
    createInfo.setBindings(bindings);

    auto [insertedIt, success] = m_cache.try_emplace(
        std::move(bindings), vk::raii::DescriptorSetLayout { *m_device, createInfo });

    return insertedIt->second;
}
}
