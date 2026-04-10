#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <vector>
#include <utility>

namespace Vulkan {
class CDescriptorAllocator {
public:
    struct PoolSizes {
        std::vector<std::pair<vk::DescriptorType, float>> sizes = {
            { vk::DescriptorType::eSampler, 0.5f },
            { vk::DescriptorType::eCombinedImageSampler, 4.0f },
            { vk::DescriptorType::eSampledImage, 4.0f },
            { vk::DescriptorType::eStorageImage, 1.0f },
            { vk::DescriptorType::eUniformBuffer, 2.0f },
            { vk::DescriptorType::eStorageBuffer, 2.0f },
            { vk::DescriptorType::eUniformBufferDynamic, 1.0f },
            { vk::DescriptorType::eStorageBufferDynamic, 1.0f },
            { vk::DescriptorType::eInputAttachment, 0.5f }
        };
    };

    explicit CDescriptorAllocator(std::nullptr_t) {}
    explicit CDescriptorAllocator(const CContext& context);
    CDescriptorAllocator(const CDescriptorAllocator&) = delete;
    CDescriptorAllocator(CDescriptorAllocator&&) noexcept = default;
    CDescriptorAllocator& operator=(const CDescriptorAllocator&) = delete;
    CDescriptorAllocator& operator=(CDescriptorAllocator&&) noexcept = default;
    ~CDescriptorAllocator() = default;

    void ResetPools();

    std::vector<vk::raii::DescriptorSet> Allocate(const vk::ArrayProxyNoTemporaries<const vk::DescriptorSetLayout>& layouts);

private:
    const vk::raii::Device* m_device;
    vk::raii::DescriptorPool m_currentPool { nullptr };
    PoolSizes m_descriptorSizes;

    std::vector<vk::raii::DescriptorPool> m_usedPools;
    std::vector<vk::raii::DescriptorPool> m_freePools;

    vk::raii::DescriptorPool GrabPool();
    vk::raii::DescriptorPool CreatePool(std::uint32_t count);
};
}
