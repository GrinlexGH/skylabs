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

    [[nodiscard]] auto operator*() const noexcept -> vk::Image { return *m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::Image* { return &*m_handle; }

    [[nodiscard]] auto View() const noexcept -> const vk::raii::ImageView& { return m_view; }
    [[nodiscard]] auto Layout() const noexcept -> vk::ImageLayout { return m_layout; }
    [[nodiscard]] auto Format() const noexcept -> vk::Format { return m_format; }
    [[nodiscard]] auto Extent() const noexcept -> vk::Extent2D { return m_extent; }
    [[nodiscard]] auto MipLevels() const noexcept -> std::uint32_t { return m_mipLevels; }
    [[nodiscard]] auto SampleCount() const noexcept -> vk::SampleCountFlagBits { return m_sampleCount; }

    auto Clear() -> void;

    static auto CmdTransitionLayout(
        const vk::raii::CommandBuffer& cmd,
        vk::Image image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor,
        std::uint32_t mipLevels = 1
    ) -> void;

    auto TransitionLayout(const vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout) -> void;

    auto CopyBufferToImage(const vk::raii::CommandBuffer& commandBuffer, const vk::Buffer& buffer, const vk::Extent2D& extent) const -> void;

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
