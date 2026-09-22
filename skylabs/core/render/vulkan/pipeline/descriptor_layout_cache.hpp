#pragma once
#include <boost/unordered/unordered_map.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace sk::render::vulkan {
struct DescriptorLayoutHash {
    std::size_t operator()(const std::vector<vk::DescriptorSetLayoutBinding>& bindings) const;
};

class DescriptorLayoutCache {
public:
    explicit DescriptorLayoutCache(std::nullptr_t) { }
    explicit DescriptorLayoutCache(const vk::raii::Device& device);

    [[nodiscard]] const vk::raii::DescriptorSetLayout& GetLayout(
        std::vector<vk::DescriptorSetLayoutBinding> bindings);
    void Clear() { m_cache.clear(); }

private:
    const vk::raii::Device* m_device = nullptr;

    boost::unordered_map<std::vector<vk::DescriptorSetLayoutBinding>, vk::raii::DescriptorSetLayout,
                         DescriptorLayoutHash>
        m_cache;
};
}
