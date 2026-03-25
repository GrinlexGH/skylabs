#include <skylabs/core/render/vulkan/render_graph/buffer_pool.hpp>

namespace Vulkan {
CBufferPool::CBufferPool(
    const CContext& context,
    std::uint32_t inFlightCount
) : m_context(&context), m_inFlightCount(inFlightCount)
{ }

BufferHandle CBufferPool::CreateBuffer(const char* debugName, const BufferDescirption& description) {
    BufferHandle handle { static_cast<unsigned int>(m_buffers.size()) };
    m_buffers.emplace_back(debugName, description);
    return handle;
}

BufferHandle CBufferPool::ImportBuffer(const char* debugName, CBuffer buffer) {
    std::vector<CBuffer> vec;
    vec.push_back(std::move(buffer));
    return ImportBuffer(debugName, std::move(vec));
}

BufferHandle CBufferPool::ImportBuffer(const char* debugName, std::vector<CBuffer> buffers) {
    std::size_t count = buffers.size();

    if (count != 1 && count != m_inFlightCount) {
        throw std::runtime_error(fmt::format(
            "BufferPool Error: '{}' import count ({}) doesn't match inFlightCount ({})", 
            debugName, count, m_inFlightCount));
    }

    BufferHandle handle { static_cast<unsigned int>(m_buffers.size()) };
    Buffer entry;
    entry.m_debugName = debugName;
    entry.m_description.m_size = buffers[0].Size();
    entry.m_description.m_isInFlight = (count > 1);
    entry.m_buffers = std::move(buffers);

    m_buffers.push_back(std::move(entry));
    return handle;
}

void CBufferPool::GenerateBuffers() {
    const bool hasDebug =
        m_context->Instance().IsExtensionEnabled(vk::EXTDebugUtilsExtensionName);

    auto createBuffer = [this](BufferDescirption meta) {
        return CBuffer {
            *m_context,
            meta.m_size,
            meta.m_usage,
            meta.m_location
        };
    };

    for (auto& [debugName, desc, buffers] : m_buffers) {
        if (!buffers.empty())
            continue;

        if (desc.m_isInFlight) {
            buffers.clear();
            buffers.reserve(m_inFlightCount);

            for (std::size_t i = 0; i < m_inFlightCount; ++i) {
                CBuffer buffer = createBuffer(desc);

                if (hasDebug)
                    m_context->Device()->setDebugUtilsObjectNameEXT(**buffer, fmt::format("{}-{}", debugName, i));

                buffers.push_back(std::move(buffer));
            }
        } else {
            buffers.push_back(createBuffer(desc));
        }
    }
}

CBuffer& CBufferPool::GetBuffer(BufferHandle handle, int index) {
    auto& entry = m_buffers.at(handle.m_id);

    if (entry.m_buffers.size() == 1) {
        return entry.m_buffers[0];
    }

    const std::uint32_t finalIndex = (index == -1) ? m_frameIndex : static_cast<uint32_t>(index);
    return entry.m_buffers[finalIndex % m_inFlightCount];
}
}
