#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>
#include <skylabs/core/render/vulkan/resources/buffer.hpp>
#include <skylabs/core/render/vulkan/command_recording/sync_state.hpp>

#include <variant>

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

struct BufferCopyOffsets
{
    vk::DeviceSize m_srcOffset = 0;
    vk::DeviceSize m_dstOffset = 0;
};

class CCommandBuffer
{
public:
    explicit CCommandBuffer(std::nullptr_t) {}
    explicit CCommandBuffer(const CDeviceContext& context, vk::raii::CommandBuffer&& commandBuffer);

    [[nodiscard]] const vk::raii::CommandBuffer& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::CommandBuffer* operator->() const noexcept { return &m_handle; }

    template <typename F>
    requires requires(const F& f, const CCommandBuffer& cmd) { { f(cmd) } -> std::same_as<void>; }
    void ImmediateSubmit(const vk::raii::Queue& queue, const F& func) const {
        m_handle.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        func(*this);
        m_handle.end();

        vk::SubmitInfo submitInfo {};
        submitInfo.setCommandBuffers(*m_handle);

        vk::raii::Fence fence { *m_device, vk::FenceCreateInfo {} };
        queue.submit(submitInfo, *fence);

        if (m_device->waitForFences({ *fence }, true, std::numeric_limits<std::uint64_t>::max()) != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to wait for single-time command fence");
        }
    }

    void PipelineBarrier(const std::vector<std::variant<ImageBarrier, BufferBarrier>>& barriers) const;

    void Copy(const CImage& dst, const CBuffer& src) const;
    void Copy(const CBuffer& dst, const CBuffer& src, vk::DeviceSize size, BufferCopyOffsets offsets = {}) const;

    void GenerateMipmaps(const CImage& image, Usage srcUsage = Usage::eTransferWrite, Usage dstUsage = Usage::eSampledFragment) const;

private:
    const vk::raii::Device* m_device = nullptr;
    vk::raii::CommandBuffer m_handle { nullptr };
};
}
