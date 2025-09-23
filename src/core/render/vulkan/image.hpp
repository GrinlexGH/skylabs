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
    CImage& operator=(CImage&& rhs) noexcept {
        if (this != &rhs) {
            if (m_handle) { Destroy(); }
            m_handle = std::exchange(rhs.m_handle, nullptr);
            m_allocation = std::exchange(rhs.m_allocation, nullptr);
            m_view = std::exchange(rhs.m_view, nullptr);
            m_layout = std::exchange(rhs.m_layout, vk::ImageLayout::eUndefined);
            m_context = std::exchange(rhs.m_context, nullptr);
        }
        return *this;
    }
    ~CImage();

    [[nodiscard]] auto GetHandle() const -> vk::Image { return m_handle; }
    [[nodiscard]] auto GetView() const -> const vk::raii::ImageView& { return m_view; }

    auto TransitionLayout(const vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout) -> void;
    auto CopyBufferToImage(
        const vk::raii::CommandBuffer& commandBuffer,
        const vk::raii::Buffer& buffer,
        vk::Extent3D extent
    ) -> void;

private:
    auto Destroy() -> void;

    vk::Image m_handle = nullptr;
    vma::Allocation m_allocation = nullptr;
    vk::raii::ImageView m_view = nullptr;

    vk::ImageLayout m_layout = vk::ImageLayout::eUndefined;

    const CContext* m_context;
};
}
