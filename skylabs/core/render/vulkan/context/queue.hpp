#pragma once
#include <skylabs/core/pch.hpp>

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

    [[nodiscard]] const vk::raii::Queue& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Queue* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] std::uint32_t FamilyIndex() const noexcept { return m_familyIndex; }

private:
    vk::raii::Queue m_handle { nullptr };
    std::uint32_t m_familyIndex = 0;
};
}
