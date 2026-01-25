#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CImage
{
public:
    explicit CImage(std::nullptr_t) {}
    CImage(
        const CContext& context,
        const vk::Extent2D& extent,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::ImageAspectFlags imageAspectFlags,
        std::uint32_t mipLevels = 1,
        vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1
    );
    CImage(const CImage&) = delete;
    CImage(CImage&&) noexcept = default;
    CImage& operator=(const CImage&) = delete;
    CImage& operator=(CImage&&) noexcept = default;
    ~CImage() = default;

    [[nodiscard]] vk::Image operator*() const noexcept { return *m_handle; }
    [[nodiscard]] const vk::Image* operator->() const noexcept { return &*m_handle; }

    [[nodiscard]] const vk::raii::ImageView& View() const noexcept { return m_view; }
    [[nodiscard]] vk::ImageLayout Layout() const noexcept { return m_layout; }
    [[nodiscard]] vk::Format Format() const noexcept { return m_format; }
    [[nodiscard]] vk::Extent2D Extent() const noexcept { return m_extent; }
    [[nodiscard]] std::uint32_t MipLevels() const noexcept { return m_mipLevels; }
    [[nodiscard]] vk::SampleCountFlagBits SampleCount() const noexcept { return m_sampleCount; }

    void SetLayout(const vk::ImageLayout layout) noexcept { m_layout = layout; }

    void Clear();

    static void CmdTransitionLayout(
        const vk::raii::CommandBuffer& cmd,
        vk::Image image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor,
        std::uint32_t mipLevels = 1
    );

    void TransitionLayout(const vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout);

    void CopyBufferToImage(const vk::raii::CommandBuffer& commandBuffer, const vk::Buffer& buffer, const vk::Extent2D& extent) const;

private:
    vma::raii::Image m_handle { nullptr };
    vk::raii::ImageView m_view = nullptr;

    vk::Format m_format = vk::Format::eUndefined;
    vk::Extent2D m_extent;
    std::uint32_t m_mipLevels = 1;
    vk::SampleCountFlagBits m_sampleCount = vk::SampleCountFlagBits::e1;
    vk::ImageLayout m_layout = vk::ImageLayout::eUndefined;
    vk::ImageAspectFlags m_aspectFlags = vk::ImageAspectFlagBits::eNone;
};
}
