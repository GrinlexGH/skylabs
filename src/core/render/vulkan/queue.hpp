#pragma once
#include "vulkan.hpp"
#include "queue_families.hpp"

namespace Vulkan {
class CQueue
{
public:
    explicit CQueue(const vk::Device& device, uint32_t family_index, vk::QueueFamilyProperties properties, vk::Bool32 can_present, uint32_t index);

private:
    vk::Queue m_handle;

    std::uint32_t m_familyIndex = 0;

    std::uint32_t m_index = 0;

    vk::Bool32 m_canPresent = false;

    vk::QueueFamilyProperties m_properties {};
};
}
