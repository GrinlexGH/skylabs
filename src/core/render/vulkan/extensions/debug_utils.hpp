#pragma once
#include "../vulkan.hpp"

namespace Vulkan {
class CDebugUtils
{
public:
    explicit CDebugUtils(const vk::Instance& instance);

    static vk::DebugUtilsMessengerCreateInfoEXT& CreateInfo();
    [[nodiscard]] vk::DebugUtilsMessengerEXT GetMessenger() const { return m_messenger; }

private:

    vk::DebugUtilsMessengerEXT m_messenger { VK_NULL_HANDLE };
};
}
