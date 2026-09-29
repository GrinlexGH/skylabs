#pragma once
#include <unordered_map>

#include <vulkan/vulkan_raii.hpp>

namespace sk::render::vulkan {
struct PipelineLayoutInfo {
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<vk::PushConstantRange> pushConstants;

    bool operator==(const PipelineLayoutInfo& rhs) const;
};

struct PipelineLayoutHash {
    std::size_t operator()(const PipelineLayoutInfo& info) const;
};

class PipelineLayoutCache {
public:
    explicit PipelineLayoutCache(std::nullptr_t) { }
    explicit PipelineLayoutCache(const vk::raii::Device& device);

    const vk::raii::PipelineLayout& GetLayout(PipelineLayoutInfo layoutInfo);
    void Clear() { m_cache.clear(); }

private:
    const vk::raii::Device* m_device = nullptr;

    std::unordered_map<PipelineLayoutInfo, vk::raii::PipelineLayout, PipelineLayoutHash> m_cache;
};
}
