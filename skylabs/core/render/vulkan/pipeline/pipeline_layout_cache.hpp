#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
struct PipelineLayoutInfo {
    std::vector<vk::DescriptorSetLayout> m_descriptorSetLayouts {};
    std::vector<vk::PushConstantRange> m_pushConstants {};

    bool operator==(const PipelineLayoutInfo& other) const {
        return m_descriptorSetLayouts == other.m_descriptorSetLayouts && m_pushConstants == other.m_pushConstants;
    }
};

struct PipelineLayoutHash {
    std::size_t operator()(const PipelineLayoutInfo& info) const;
};

class CPipelineLayoutCache
{
public:
    explicit CPipelineLayoutCache(std::nullptr_t) {}
    explicit CPipelineLayoutCache(const CContext& context);

    const vk::raii::PipelineLayout& GetLayout(PipelineLayoutInfo layoutInfo);
    void Clear() { m_cache.clear(); }

private:
    const vk::raii::Device* m_device { nullptr };

    std::unordered_map<PipelineLayoutInfo, vk::raii::PipelineLayout, PipelineLayoutHash> m_cache;
};
}
