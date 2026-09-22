#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace sk::render::vulkan {
class DescriptorAllocator {
public:
    explicit DescriptorAllocator(std::nullptr_t) { }
    explicit DescriptorAllocator(const vk::raii::Device& device);
    DescriptorAllocator(const DescriptorAllocator&) = delete;
    DescriptorAllocator(DescriptorAllocator&&) noexcept = default;
    DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;
    DescriptorAllocator& operator=(DescriptorAllocator&&) noexcept = default;
    ~DescriptorAllocator() = default;

    void ResetPools();

    std::vector<vk::raii::DescriptorSet> Allocate(
        const vk::ArrayProxy<const vk::DescriptorSetLayout>& layouts);

private:
    const vk::raii::Device* m_device = nullptr;
    vk::raii::DescriptorPool m_currentPool { nullptr };

    std::vector<vk::raii::DescriptorPool> m_usedPools;
    std::vector<vk::raii::DescriptorPool> m_freePools;

    vk::raii::DescriptorPool GrabPool();
    vk::raii::DescriptorPool CreatePool(std::uint32_t count);
    std::expected<std::vector<vk::raii::DescriptorSet>, vk::Result> Allocate(
        const vk::DescriptorSetAllocateInfo& allocInfo) const;
};
}
