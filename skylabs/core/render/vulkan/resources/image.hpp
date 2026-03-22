#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
enum class Usage : std::uint8_t
{
    eNone,
    eColorAttachment,
    eDepthWrite,
    eDepthRead,
    eSampledFragment,
    eComputeWrite,
    ePresent
};

struct ImageSyncState
{
    vk::PipelineStageFlags2 m_stage = vk::PipelineStageFlagBits2::eNone;
    vk::AccessFlags2 m_access = vk::AccessFlagBits2::eNone;
    vk::ImageLayout m_layout = vk::ImageLayout::eUndefined;
    std::uint32_t m_queue = vk::QueueFamilyIgnored;
    Usage m_usage = Usage::eNone;

    static inline const ImageSyncState ColorWrite() { return { vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal }; }
    static inline const ImageSyncState DepthWrite() { return { vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal }; }
    static inline const ImageSyncState ShaderRead() { return { vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal }; }
    static inline const ImageSyncState ComputeWrite() { return { vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite, vk::ImageLayout::eGeneral }; }
    static inline const ImageSyncState Present() { return { vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, vk::ImageLayout::ePresentSrcKHR }; }
};

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
    friend class CCommandBuffer;

public:
    explicit CImage(std::nullptr_t) {}
    explicit CImage(const CContext& context, ImageCreateInfo options = {});
    CImage(const CImage&) = delete;
    CImage(CImage&&) noexcept = default;
    CImage& operator=(const CImage&) = delete;
    CImage& operator=(CImage&&) noexcept = default;
    ~CImage() = default;

    [[nodiscard]] vk::Image operator*() const noexcept { return *m_handle; }
    [[nodiscard]] const vk::Image* operator->() const noexcept { return &*m_handle; }

    [[nodiscard]] const vk::raii::ImageView& View() const noexcept { return m_view; }
    [[nodiscard]] vk::Format Format() const noexcept { return m_format; }
    [[nodiscard]] vk::Extent3D Extent() const noexcept { return m_extent; }
    [[nodiscard]] std::uint32_t MipLevels() const noexcept { return m_mipLevels; }
    [[nodiscard]] std::uint32_t ArrayLevels() const noexcept { return m_arrayLevels; }
    [[nodiscard]] vk::SampleCountFlagBits SampleCount() const noexcept { return m_sampleCount; }
    [[nodiscard]] vk::ImageAspectFlags AspectFlags() const noexcept { return m_aspectFlags; }
    [[nodiscard]] vk::ImageSubresourceRange FullRange() const noexcept { return { m_aspectFlags, 0, m_mipLevels, 0, m_arrayLevels }; }
    [[nodiscard]] ImageSyncState SyncState() const noexcept { return m_syncState; }

    void SetSyncState(ImageSyncState state) noexcept { m_syncState = state; }

    void Clear();

    void CopyBufferToImage(const vk::CommandBuffer& commandBuffer, const vk::Buffer& buffer, const vk::Extent2D& extent) const;

private:
    vma::raii::Image m_handle { nullptr };
    vk::raii::ImageView m_view { nullptr };

    vk::Format m_format = vk::Format::eUndefined;
    vk::Extent3D m_extent {};
    std::uint32_t m_mipLevels = 1;
    std::uint32_t m_arrayLevels = 1;
    vk::SampleCountFlagBits m_sampleCount = vk::SampleCountFlagBits::e1;
    vk::ImageAspectFlags m_aspectFlags = vk::ImageAspectFlagBits::eNone;

    ImageSyncState m_syncState {};
};
}
