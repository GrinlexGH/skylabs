#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>
#include <skylabs/core/render/vulkan/resources/buffer.hpp>

namespace Vulkan {
struct ImageBarrierInfo
{
    const CImage& m_image;
    Usage m_oldUsage = Usage::eNone;
    Usage m_newUsage = Usage::eNone;
};

class CCommandBuffer
{
public:
    explicit CCommandBuffer(std::nullptr_t) {}
    explicit CCommandBuffer(const vk::raii::CommandBuffer& commandBuffer);

    [[nodiscard]] const vk::raii::CommandBuffer& operator*() const noexcept { return *m_handle; }
    [[nodiscard]] const vk::raii::CommandBuffer* operator->() const noexcept { return m_handle; }

    void PipelineBarrier(std::vector<ImageBarrierInfo> imageBarriers) const;

    void ReleaseOwnership(const CImage& image, std::uint32_t srcQueue, std::uint32_t dstQueue, Usage oldUsage, Usage newUsage) const;
    void AcquireOwnership(CImage& image, std::uint32_t srcQueue, std::uint32_t dstQueue, Usage oldUsage, Usage newUsage) const;

    void Copy(const CImage& dst, const CBuffer& src) const;

private:
    const vk::raii::CommandBuffer* m_handle = nullptr;
};
}
