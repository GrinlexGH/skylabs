#pragma once
#include <variant>

#include "../../../../engine/render/vulkan/image.hpp"
#include "skylabs/core/render/vulkan/command_recording/sync_state.hpp"
#include "skylabs/core/render/vulkan/resources/buffer.hpp"

namespace sk::render::vulkan {
enum class BarrierType : std::uint8_t { eRegular, eRelease, eAcquire };

struct ImageBarrier {
    const Image& image;
    vk::ImageSubresourceRange range { };
    Usage oldUsage = Usage::eNone;
    Usage newUsage = Usage::eNone;
    BarrierType type = BarrierType::eRegular;
    std::uint32_t srcQueue = vk::QueueFamilyIgnored;
    std::uint32_t dstQueue = vk::QueueFamilyIgnored;
};

struct BufferBarrier {
    const Buffer& buffer;
    Usage oldUsage = Usage::eNone;
    Usage newUsage = Usage::eNone;
    BarrierType type = BarrierType::eRegular;
    std::uint32_t srcQueue = vk::QueueFamilyIgnored;
    std::uint32_t dstQueue = vk::QueueFamilyIgnored;
};

struct BufferCopyOffsets {
    vk::DeviceSize srcOffset = 0;
    vk::DeviceSize dstOffset = 0;
};

class CommandBuffer {
public:
    explicit CommandBuffer(std::nullptr_t) { }
    explicit CommandBuffer(const vk::raii::Device& device, vk::raii::CommandBuffer&& commandBuffer);

    [[nodiscard]] const vk::raii::CommandBuffer& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::CommandBuffer* operator->() const noexcept { return &m_handle; }

    template <typename F>
        requires requires(const F& f, const CommandBuffer& cmd) {
            { f(cmd) } -> std::same_as<void>;
        }
    void ImmediateSubmit(const vk::raii::Queue& queue, const F& func) const {
        m_handle.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        func(*this);
        m_handle.end();

        vk::SubmitInfo submitInfo { };
        submitInfo.setCommandBuffers(*m_handle);

        const vk::raii::Fence fence { *m_device, vk::FenceCreateInfo { } };
        queue.submit(submitInfo, *fence);

        if (m_device->waitForFences({ *fence }, true, std::numeric_limits<std::uint64_t>::max()) !=
            vk::Result::eSuccess) {
            throw std::runtime_error("Failed to wait for single-time command fence");
        }
    }

    void PipelineBarrier(const std::vector<std::variant<ImageBarrier, BufferBarrier>>& barriers) const;

    void Copy(const Buffer& src, const Image& dst) const;
    void Copy(const Buffer& src, const Buffer& dst, vk::DeviceSize size,
              const BufferCopyOffsets& offsets = { }) const;

    void GenerateMipmaps(const Image& image, Usage srcUsage = Usage::eTransferWrite,
                         Usage dstUsage = Usage::eSampledFragment) const;

private:
    const vk::raii::Device* m_device = nullptr;
    vk::raii::CommandBuffer m_handle { nullptr };
};
}
