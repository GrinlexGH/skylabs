#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CCommandBuffer
{
public:
    explicit CCommandBuffer(std::nullptr_t) {}
    explicit CCommandBuffer(const vk::raii::CommandBuffer& commandBuffer);

    [[nodiscard]] const vk::raii::CommandBuffer& operator*() const noexcept { return *m_handle; }
    [[nodiscard]] const vk::raii::CommandBuffer* operator->() const noexcept { return m_handle; }

private:
    const vk::raii::CommandBuffer* m_handle = nullptr;
};
}
