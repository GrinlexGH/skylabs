#include "queue.hpp"

namespace Vulkan {
CQueue::CQueue(const vk::Device& device, const std::uint32_t familyIndex) :
    m_familyIndex(familyIndex)
{
    m_handle = device.getQueue(familyIndex, 0);
}
}
