#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
struct DescriptorLayoutInfo {
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
    bool operator==(const DescriptorLayoutInfo& other) const { return m_bindings == other.m_bindings; }
};

struct DescriptorLayoutHash {
    std::size_t operator()(const DescriptorLayoutInfo& info) const;
};

class CDescriptorLayoutCache {
public:
    explicit CDescriptorLayoutCache(std::nullptr_t) {}
    explicit CDescriptorLayoutCache(const CContext& context);

    const vk::raii::DescriptorSetLayout& GetLayout(DescriptorLayoutInfo info);
    void Clear() { m_cache.clear(); }

private:
    const CContext* m_context = nullptr;

    std::unordered_map<DescriptorLayoutInfo, vk::raii::DescriptorSetLayout, DescriptorLayoutHash> m_cache;
};
}
