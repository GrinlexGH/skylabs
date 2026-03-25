#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>
#include <skylabs/core/render/vulkan/resources/buffer.hpp>

#include <variant>
#include <optional>

namespace Vulkan {
enum class BarrierType : std::uint8_t
{
    eRegular,
    eRelease,
    eAcquire
};

struct ImageBarrier
{
    const CImage& m_image;
    vk::ImageSubresourceRange m_range {};
    Usage m_oldUsage = Usage::eNone;
    Usage m_newUsage = Usage::eNone;
    BarrierType m_type = BarrierType::eRegular;
    std::uint32_t srcQueue = vk::QueueFamilyIgnored;
    std::uint32_t dstQueue = vk::QueueFamilyIgnored;
};

struct BufferBarrier
{
    const CBuffer& m_buffer;
    Usage m_oldUsage = Usage::eNone;
    Usage m_newUsage = Usage::eNone;
    BarrierType m_type = BarrierType::eRegular;
    std::uint32_t srcQueue = vk::QueueFamilyIgnored;
    std::uint32_t dstQueue = vk::QueueFamilyIgnored;
};

class CCommandBuffer
{
public:
    explicit CCommandBuffer(std::nullptr_t) {}
    explicit CCommandBuffer(const vk::raii::CommandBuffer& commandBuffer);

    [[nodiscard]] const vk::raii::CommandBuffer& operator*() const noexcept { return *m_handle; }
    [[nodiscard]] const vk::raii::CommandBuffer* operator->() const noexcept { return m_handle; }

    void PipelineBarrier(const std::vector<std::variant<ImageBarrier, BufferBarrier>>& barriers) const;

    void Copy(const CImage& dst, const CBuffer& src) const;
    void Copy(const CBuffer& dst, const CBuffer& src, vk::DeviceSize size) const;

    void GenerateMipmaps(const CImage& image, Usage srcUsage = Usage::eTransferWrite, Usage dstUsage = Usage::eSampledFragment) const;

private:
    const vk::raii::CommandBuffer* m_handle = nullptr;
};
}
