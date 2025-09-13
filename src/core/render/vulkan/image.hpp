#pragma once
#include "context/context.hpp"

namespace Vulkan {
class CImage
{
public:
    explicit CImage(std::nullptr_t);
    CImage(
        const CContext* context,
        vk::Extent3D extent,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::ImageAspectFlags imageAspectFlags,
        vk::MemoryPropertyFlags memoryProperties
    );
    CImage(const CImage&) = delete;
    CImage(CImage&&) noexcept = default;
    CImage& operator=(const CImage&) = delete;
    CImage& operator=(CImage&&) noexcept = default;
    ~CImage() = default;

    [[nodiscard]] auto GetHandle() const -> const vk::raii::Image& { return m_handle; }
    [[nodiscard]] auto GetView() const -> const vk::raii::ImageView& { return m_view; }

    auto TransitionLayout(const vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout) -> void;
    auto CopyBufferToImage(
        const vk::raii::CommandBuffer& commandBuffer,
        const vk::raii::Buffer& buffer,
        vk::Extent3D extent
    ) -> void;

private:
    vk::raii::Image m_handle = nullptr;
    vk::raii::DeviceMemory m_memory = nullptr;
    vk::raii::ImageView m_view = nullptr;

    vk::ImageLayout m_layout = vk::ImageLayout::eUndefined;

    const CContext* m_context;
};
}
