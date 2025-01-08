#pragma once
#include "vulkan.hpp"
#include "queue_families.hpp"

namespace Vulkan
{
class CQueues
{
public:
    void Init(vk::Device device, const CQueueFamilies& families);

    vk::Queue m_graphics;
    vk::Queue m_present;
    vk::Queue m_transfer;
    vk::Queue m_compute;
};
}
