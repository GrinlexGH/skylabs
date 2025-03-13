#pragma once
#include "vulkan.hpp"

namespace Vulkan {
class CQueue
{
public:
    explicit CQueue(const vk::Device& device, std::uint32_t familyIndex);

    [[nodiscard]] std::uint32_t GetFamilyIndex() const { return m_familyIndex; }

private:
    vk::Queue m_handle;

    std::uint32_t m_familyIndex;
};
}
