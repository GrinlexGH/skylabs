#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>

namespace Vulkan {
enum class BarrierType : std::uint8_t
{
    Regular,
    Release,
    Acquire
};

struct ImageBarrierInfo {
    vk::Image m_image;
    vk::ImageLayout m_oldLayout;
    vk::ImageLayout m_newLayout;
    vk::ImageAspectFlags m_aspectMask = vk::ImageAspectFlagBits::eColor;
    uint32_t m_mipLevels = 1;
    uint32_t m_arrayLevels = 1;
    uint32_t m_srcQueue = vk::QueueFamilyIgnored;
    uint32_t m_dstQueue = vk::QueueFamilyIgnored;

    CImage* m_sourceCImage = nullptr;

    ImageBarrierInfo(CImage& img, vk::ImageLayout nextLayout)
        : m_image(*img), m_oldLayout(img.Layout()), m_newLayout(nextLayout),
          m_aspectMask(img.AspectFlags()), m_mipLevels(img.MipLevels()),
          m_arrayLevels(img.ArrayLevels()), m_sourceCImage(&img) {}

    ImageBarrierInfo(CImage& img, uint32_t srcQ, uint32_t dstQ, vk::ImageLayout nextLayout)
        : m_image(*img), m_oldLayout(img.Layout()), m_newLayout(nextLayout),
          m_aspectMask(img.AspectFlags()), m_mipLevels(img.MipLevels()),
          m_arrayLevels(img.ArrayLevels()), m_srcQueue(srcQ), m_dstQueue(dstQ),
          m_sourceCImage(&img) {}

    ImageBarrierInfo(vk::Image img, vk::ImageLayout oldL, vk::ImageLayout newL,
                     vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor)
        : m_image(img), m_oldLayout(oldL), m_newLayout(newL), m_aspectMask(aspect) {}
};

class CCommandBuffer
{
public:
    explicit CCommandBuffer(std::nullptr_t) {}
    explicit CCommandBuffer(const vk::raii::CommandBuffer& commandBuffer);

    [[nodiscard]] const vk::raii::CommandBuffer& operator*() const noexcept { return *m_handle; }
    [[nodiscard]] const vk::raii::CommandBuffer* operator->() const noexcept { return m_handle; }

    void TransitionLayout(std::initializer_list<ImageBarrierInfo> transitions) const;
    void TransitionLayout(CImage& image, vk::ImageLayout newLayout) const;
    void TransitionLayout(
        vk::Image image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor,
        std::uint32_t mipLevels = 1,
        std::uint32_t arrayLevels = 1
    ) const;

    void ReleaseOwnership(std::initializer_list<ImageBarrierInfo> releases) const;
    void ReleaseOwnership(const CImage& image, std::uint32_t srcQueue, std::uint32_t dstQueue, vk::ImageLayout newLayout) const;

    void AcquireOwnership(std::initializer_list<ImageBarrierInfo> acquires) const;
    void AcquireOwnership(CImage& image, std::uint32_t srcQueue, std::uint32_t dstQueue, vk::ImageLayout newLayout) const;

private:
    const vk::raii::CommandBuffer* m_handle = nullptr;

    vk::ImageMemoryBarrier2 CreateImageBarrier(
        vk::Image image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::ImageAspectFlags aspectMask,
        uint32_t mipLevels,
        uint32_t arrayLayers
    ) const;
};
}
