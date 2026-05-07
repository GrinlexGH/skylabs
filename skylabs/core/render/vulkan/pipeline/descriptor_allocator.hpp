#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <vector>

namespace Vulkan {
class CDescriptorAllocator {
public:
    explicit CDescriptorAllocator(std::nullptr_t) {}
    explicit CDescriptorAllocator(const CDeviceContext& context);
    CDescriptorAllocator(const CDescriptorAllocator&) = delete;
    CDescriptorAllocator(CDescriptorAllocator&&) noexcept = default;
    CDescriptorAllocator& operator=(const CDescriptorAllocator&) = delete;
    CDescriptorAllocator& operator=(CDescriptorAllocator&&) noexcept = default;
    ~CDescriptorAllocator() = default;

    void ResetPools();

    std::vector<vk::raii::DescriptorSet> Allocate(const vk::ArrayProxy<const vk::DescriptorSetLayout>& layouts);

private:
    const vk::raii::Device* m_device = nullptr;
    vk::raii::DescriptorPool m_currentPool { nullptr };

    std::vector<vk::raii::DescriptorPool> m_usedPools;
    std::vector<vk::raii::DescriptorPool> m_freePools;

    vk::raii::DescriptorPool GrabPool();
    vk::raii::DescriptorPool CreatePool(std::uint32_t count);
    std::expected<std::vector<vk::raii::DescriptorSet>, vk::Result> Allocate(const vk::DescriptorSetAllocateInfo& allocInfo) const;
};
}
