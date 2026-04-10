#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
struct DescriptorLayoutHash {
    std::size_t operator()(const std::vector<vk::DescriptorSetLayoutBinding>& info) const;
};

class CDescriptorLayoutCache {
public:
    explicit CDescriptorLayoutCache(std::nullptr_t) {}
    explicit CDescriptorLayoutCache(const CContext& context);

    [[nodiscard]] const vk::raii::DescriptorSetLayout& GetLayout(std::vector<vk::DescriptorSetLayoutBinding> bindings);
    void Clear() { m_cache.clear(); }

private:
    const vk::raii::Device* m_device = nullptr;

    std::unordered_map<std::vector<vk::DescriptorSetLayoutBinding>, vk::raii::DescriptorSetLayout, DescriptorLayoutHash> m_cache;
};
}
