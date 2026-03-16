#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

#include <boost/container_hash/hash.hpp>

namespace Vulkan {
struct DescriptorLayoutInfo {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bool operator==(const DescriptorLayoutInfo& other) const { return bindings == other.bindings; }
};

struct DescriptorLayoutHash {
    size_t operator()(const DescriptorLayoutInfo& info) const {
        size_t seed = 0;
        for (const auto& b : info.bindings) {
            boost::hash_combine(seed, b.binding);
            boost::hash_combine(seed, static_cast<uint32_t>(b.descriptorType));
            boost::hash_combine(seed, b.descriptorCount);
            boost::hash_combine(seed, static_cast<uint32_t>(b.stageFlags));
            boost::hash_combine(seed, b.pImmutableSamplers);
        }
        return seed;
    }
};

class CDescriptorLayoutCache {
public:
    explicit CDescriptorLayoutCache(std::nullptr_t) {}
    explicit CDescriptorLayoutCache(const CContext& context);

    const vk::raii::DescriptorSetLayout& GetLayout(std::vector<vk::DescriptorSetLayoutBinding> bindings);
    void Clear() { m_cache.clear(); }

private:
    const CContext* m_context = nullptr;

    std::unordered_map<DescriptorLayoutInfo, vk::raii::DescriptorSetLayout, DescriptorLayoutHash> m_cache;
};
}
