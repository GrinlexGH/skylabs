#include <skylabs/core/render/vulkan/memory/memory_mapping.hpp>

namespace Vulkan {
CBufferMapping::CBufferMapping(vma::raii::Buffer& buffer) : m_buffer(&buffer) {
    m_data = buffer.getAllocation().map();
}

CBufferMapping::~CBufferMapping() {
    if (m_data && m_buffer) {
        m_buffer->getAllocation().unmap();
    }
}
}
