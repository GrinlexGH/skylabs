#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CImage
{
public:
    explicit CImage(std::nullptr_t) {}
    CImage(
        const CContext& context,
        const vk::Extent3D& extent,
        vk::Format format,
        vk::ImageTiling tiling,
        const vk::ImageUsageFlags& usage,
        const vk::ImageAspectFlags& imageAspectFlags,
        const vk::MemoryPropertyFlags& memoryProperties,
        std::uint32_t mipLevels = 1
    );
    CImage(const CImage&) = delete;
    CImage(CImage&&) noexcept = default;
    CImage& operator=(const CImage&) = delete;
    CImage& operator=(CImage&&) noexcept = default;
    ~CImage();

    [[nodiscard]] auto operator*() const noexcept -> vk::Image { return *m_handle; }
    [[nodiscard]] auto GetHandle() const noexcept -> vk::Image { return *m_handle; }

    [[nodiscard]] auto GetView() const noexcept -> const vk::raii::ImageView& { return m_view; }
    [[nodiscard]] auto GetLayout() const noexcept -> vk::ImageLayout { return m_layout; }
    [[nodiscard]] auto GetFormat() const noexcept -> vk::Format { return m_format; }
    [[nodiscard]] auto GetExtent() const noexcept -> vk::Extent3D { return m_extent; }
    [[nodiscard]] auto GetMipLevels() const noexcept -> std::uint32_t { return m_mipLevels; }

    auto Clear() -> void;

    auto TransitionLayout(const vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout) -> void;
    auto CopyBufferToImage(const vk::raii::CommandBuffer& commandBuffer, const vk::Buffer& buffer, const vk::Extent3D& extent) -> void;

private:
    vma::UniqueImage m_handle;
    vma::UniqueAllocation m_allocation;
    vk::raii::ImageView m_view = nullptr;

    vk::ImageLayout m_layout = vk::ImageLayout::eUndefined;
    vk::Format m_format = vk::Format::eUndefined;
    vk::Extent3D m_extent;
    std::uint32_t m_mipLevels = 1;
};
}
