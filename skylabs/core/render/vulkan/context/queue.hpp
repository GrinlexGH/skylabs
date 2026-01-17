#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class CQueue
{
public:
    explicit CQueue(std::nullptr_t) {}
    explicit CQueue(
        const vk::raii::Device& device,
        const std::uint32_t familyIndex,
        const std::uint32_t queueIndex
    ) : m_handle(device, familyIndex, queueIndex), m_familyIndex(familyIndex) {}
    CQueue(CQueue&) = delete;
    CQueue(CQueue&&) = default;
    CQueue& operator=(CQueue&) = delete;
    CQueue& operator=(CQueue&&) = default;
    ~CQueue() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::Queue& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::raii::Queue* { return &m_handle; }

    [[nodiscard]] auto FamilyIndex() const noexcept -> std::uint32_t { return m_familyIndex; }

private:
    vk::raii::Queue m_handle { nullptr };
    std::uint32_t m_familyIndex = 0;
};
}
