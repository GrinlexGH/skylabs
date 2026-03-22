#include <skylabs/core/render/vulkan/render_graph/buffer_pool.hpp>

#include <frozen/map.h>

namespace {
constexpr frozen::map<Vulkan::BufferUsageFlagBits, vk::BufferUsageFlagBits, 1> g_BufferUsageMap
{
    { Vulkan::BufferUsageFlagBits::eUniformBuffer, vk::BufferUsageFlagBits::eUniformBuffer },
};

vk::BufferUsageFlags GetVkBufferUsageFlags(Vulkan::BufferUsageFlags stage) {
    vk::BufferUsageFlags flags;
    for (auto const& [bit, vkBit] : g_BufferUsageMap) {
        if (stage & bit) {
            flags |= vkBit;
        }
    }
    return flags;
}
}

namespace Vulkan {
CBufferPool::CBufferPool(
    const CContext& context,
    std::uint32_t inFlightCount
) : m_context(&context), m_inFlightCount(inFlightCount)
{ }

BufferHandle CBufferPool::CreateBuffer(const char* debugName, const BufferDescirption& description) {
    BufferHandle handle { static_cast<unsigned int>(m_buffers.size()) };
    m_buffers.emplace_back(BufferMeta { .m_debugName = debugName, .m_description = description });
    return handle;
}

BufferHandle CBufferPool::ImportBuffer(const char* debugName, CBuffer buffer) {
    BufferHandle handle{ static_cast<unsigned int>(m_buffers.size()) };
    Buffer entry;
    entry.m_meta.m_debugName = debugName;
    entry.m_meta.m_description.m_size = buffer.Size();
    entry.m_meta.m_description.m_isInFlight = false;
    entry.m_buffers.push_back(std::move(buffer));
    m_buffers.push_back(std::move(entry));
    return handle;
}

void CBufferPool::GenerateBuffers() {
    const bool hasDebug =
        m_context->Instance().IsExtensionEnabled(vk::EXTDebugUtilsExtensionName);

    auto createBuffer = [this](BufferMeta meta) {
        return CBuffer {
            *m_context,
            meta.m_description.m_size,
            GetVkBufferUsageFlags(meta.m_description.m_usage),
            meta.m_description.m_location
        };
    };

    for (auto& [meta, buffers] : m_buffers) {
        if (!buffers.empty())
            continue;

        if (meta.m_description.m_isInFlight) {
            buffers.clear();
            buffers.reserve(m_inFlightCount);

            for (std::size_t i = 0; i < m_inFlightCount; ++i) {
                CBuffer buffer = createBuffer(meta);

                if (hasDebug)
                    m_context->Device()->setDebugUtilsObjectNameEXT(**buffer, fmt::format("{}-{}", meta.m_debugName, i));

                buffers.push_back(std::move(buffer));
            }
        } else {
            buffers.push_back(createBuffer(meta));
        }
    }
}

CBuffer& CBufferPool::GetBuffer(BufferHandle handle, int index) {
    auto& entry = m_buffers.at(handle.m_id);

    if (entry.m_buffers.size() == 1) {
        return entry.m_buffers[0];
    }

    return entry.m_buffers[(index == -1) ? m_frameIndex : static_cast<std::uint32_t>(index)];
}
}
