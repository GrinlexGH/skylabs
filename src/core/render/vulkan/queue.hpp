#pragma once
#include <cstdint>

#include "vulkan.hpp"

namespace Vulkan {
class CQueue
{
public:
    explicit CQueue(const vk::Device& device, std::uint32_t familyIndex);

    std::uint32_t GetFamilyIndex() const { return m_familyIndex; }

private:
    vk::Queue m_handle = VK_NULL_HANDLE;

    std::uint32_t m_familyIndex;
};
}
