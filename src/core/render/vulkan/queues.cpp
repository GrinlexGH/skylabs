#include "queues.hpp"

#include "queue_families.hpp"

namespace Vulkan {
void CQueues::Init(const vk::Device& device, const CQueueFamilies& families) {
    m_graphics = device.getQueue(*families.m_graphics, 0);
    m_present = device.getQueue(*families.m_present, 0);
    m_transfer = device.getQueue(*families.m_transfer, 0);
    m_compute = device.getQueue(*families.m_compute, 0);
}
}
