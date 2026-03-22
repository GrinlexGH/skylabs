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
    vk::ImageMemoryBarrier2 m_barrier {
        {}, {},
        {}, {},
        vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined,
        vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
        {}, {}
    };

    CImage* m_sourceCImage = nullptr;

    ImageBarrierInfo(vk::ImageMemoryBarrier2 barrier);
    ImageBarrierInfo(
        CImage& img,
        vk::PipelineStageFlags2 dstStage,
        vk::AccessFlags2 dstAccess,
        vk::ImageLayout newLayout,
        std::uint32_t srcQueue = vk::QueueFamilyIgnored,
        std::uint32_t dstQueue = vk::QueueFamilyIgnored
    );
};

class CCommandBuffer
{
public:
    explicit CCommandBuffer(std::nullptr_t) {}
    explicit CCommandBuffer(const vk::raii::CommandBuffer& commandBuffer);

    [[nodiscard]] const vk::raii::CommandBuffer& operator*() const noexcept { return *m_handle; }
    [[nodiscard]] const vk::raii::CommandBuffer* operator->() const noexcept { return m_handle; }

    void PipelineBarrier(std::vector<ImageBarrierInfo> imageBarriers) const;

    void ReleaseOwnership(const CImage& image, std::uint32_t srcQueue, std::uint32_t dstQueue, vk::ImageLayout newLayout) const;

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
