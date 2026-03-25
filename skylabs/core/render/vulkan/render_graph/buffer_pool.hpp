#pragma once
#include <skylabs/core/render/vulkan/resources/buffer.hpp>

namespace Vulkan {
struct BufferDescirption
{
    std::size_t m_size = 0;
    MemoryLocation m_location = MemoryLocation::eDeviceOnly;
    vk::BufferUsageFlags m_usage;
    bool m_isInFlight = false;
};

struct BufferHandle
{
    unsigned int m_id = ~0u;
};

class CBufferPool
{
public:
    explicit CBufferPool(std::nullptr_t) {}
    explicit CBufferPool(const CContext& context, std::uint32_t inFlightCount);
    CBufferPool(const CBufferPool&) = delete;
    CBufferPool(CBufferPool&&) noexcept = default;
    CBufferPool& operator=(const CBufferPool&) = delete;
    CBufferPool& operator=(CBufferPool&&) noexcept = default;
    ~CBufferPool() = default;

    [[nodiscard]] BufferHandle CreateBuffer(const char* debugName, const BufferDescirption& description);
    [[nodiscard]] BufferHandle ImportBuffer(const char* debugName, CBuffer buffer);
    [[nodiscard]] BufferHandle ImportBuffer(const char* debugName, std::vector<CBuffer> buffer);

    void GenerateBuffers();
    [[nodiscard]] CBuffer& GetBuffer(BufferHandle handle, int index = -1);

    void SetFrameIndex(std::uint32_t newFrameIndex) { m_frameIndex = newFrameIndex; }

private:
    const CContext* m_context = nullptr;
    std::uint32_t m_inFlightCount = 0;
    std::uint32_t m_frameIndex = 0;

    struct Buffer
    {
        std::string m_debugName;
        BufferDescirption m_description;
        std::vector<CBuffer> m_buffers;
    };

    std::vector<Buffer> m_buffers;
};
}
