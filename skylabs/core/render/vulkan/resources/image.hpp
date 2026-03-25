#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/resources/sync_state.hpp>

namespace Vulkan {
struct ImageCreateInfo
{
    vk::Extent3D m_extent {};
    vk::Format m_format = vk::Format::eR8G8B8A8Snorm;
    std::uint32_t m_mipLevels = 1;
    std::uint32_t m_arrayLevels = 1;
    vk::SampleCountFlagBits m_sampleCount = vk::SampleCountFlagBits::e1;
    vk::ImageUsageFlags m_usageFlags {};
};

class CImage
{
    friend class CRenderGraph;

public:
    explicit CImage(std::nullptr_t) {}
    explicit CImage(const CContext& context, ImageCreateInfo options = {});
    explicit CImage(const CContext& context,
        vk::Image imported,
        vk::Extent3D extent, vk::Format format,
        std::uint32_t mipLevels, std::uint32_t arrayLevels,
        vk::SampleCountFlagBits sampleCount
    );
    CImage(const CImage&) = delete;
    CImage(CImage&&) noexcept = default;
    CImage& operator=(const CImage&) = delete;
    CImage& operator=(CImage&&) noexcept = default;
    ~CImage() = default;

    [[nodiscard]] vk::Image operator*() const noexcept { return m_rawHandle; }
    [[nodiscard]] const vk::raii::Image& operator->() const noexcept { return m_handle; }

    [[nodiscard]] const vk::raii::ImageView& View() const noexcept { return m_view; }
    [[nodiscard]] vk::Format Format() const noexcept { return m_format; }
    [[nodiscard]] vk::Extent3D Extent() const noexcept { return m_extent; }
    [[nodiscard]] std::uint32_t MipLevels() const noexcept { return m_mipLevels; }
    [[nodiscard]] std::uint32_t ArrayLevels() const noexcept { return m_arrayLevels; }
    [[nodiscard]] vk::SampleCountFlagBits SampleCount() const noexcept { return m_sampleCount; }
    [[nodiscard]] vk::ImageAspectFlags AspectFlags() const noexcept { return m_aspectFlags; }
    [[nodiscard]] vk::ImageSubresourceRange FullRange() const noexcept { return { m_aspectFlags, 0, m_mipLevels, 0, m_arrayLevels }; }

    [[nodiscard]] ResourceSyncState SyncState() const noexcept { return m_syncState; }
    void SetSyncState(ResourceSyncState state) noexcept { m_syncState = state; }

private:
    void CreateView(const vk::raii::Device& device, vk::ImageViewType viewType);

    vk::Image m_rawHandle { nullptr };

    vma::raii::Image m_handle { nullptr };
    vk::raii::ImageView m_view { nullptr };

    vk::Format m_format = vk::Format::eUndefined;
    vk::Extent3D m_extent {};
    std::uint32_t m_mipLevels = 1;
    std::uint32_t m_arrayLevels = 1;
    vk::SampleCountFlagBits m_sampleCount = vk::SampleCountFlagBits::e1;
    vk::ImageAspectFlags m_aspectFlags = vk::ImageAspectFlagBits::eNone;

    ResourceSyncState m_syncState {};
};
}
