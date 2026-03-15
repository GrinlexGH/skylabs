#pragma once
#include <boost/container_hash/hash.hpp>

namespace Vulkan {
struct DescriptorSetDescription {
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;

    bool operator==(const DescriptorSetDescription& other) const {
        return m_bindings == other.m_bindings;
    }
};

struct DescriptorSetHash {
    size_t operator()(const DescriptorSetDescription& set) const {
        size_t seed = 0;
        for (const auto& binding : set.m_bindings) {
            boost::hash_combine(seed, static_cast<VkDescriptorSetLayoutBinding>(binding));
        }
        return seed;
    }
};

enum class DescriptorType : std::uint8_t
{
    eUniformBuffer = 0,
    eStorageImage,
    eCombinedImageSampler
};

struct BufferDescriptorInfo {
    BufferHandle m_buffer;
};

struct SampledImageDescriptorInfo {
    TextureHandle m_image;
    const vk::raii::Sampler* m_sampler = nullptr;
};

struct StorageImageDescriptorInfo {
    TextureHandle m_image;
};

struct DescriptorDescription
{
    DescriptorType m_type;
    vk::ShaderStageFlags m_shaderStages;
    std::variant<BufferDescriptorInfo, SampledImageDescriptorInfo, StorageImageDescriptorInfo> m_info;
};

class CDescriptorLayoutCache {
public:
    const vk::raii::DescriptorSetLayout& GetLayout(const DescriptorSetDescription& desc) {
        auto it = m_cache.find(desc);
        if (it != m_cache.end()) {
            return it->second;
        }

        // Если не нашли — создаем новый
        vk::PipelineLayoutCreateInfo createInfo({}, input.m_descriptorSets, input.m_pushConstants);
        auto [insertedIt, success] = m_cache.emplace(input, vk::raii::PipelineLayout(m_device, createInfo));

        return insertedIt->second;

        // Если такой набор биндингов уже есть — возвращаем
        // Если нет — создаем vk::raii::DescriptorSetLayout и сохраняем в map
    }
private:
    std::unordered_map<DescriptorSetDescription, vk::raii::DescriptorSetLayout, DescriptorSetHash> m_cache;
};
}
