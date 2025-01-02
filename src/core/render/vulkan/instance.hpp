#pragma once

#include "vulkan.hpp"
#include "vulkan_window.hpp"

namespace Vulkan
{
class CInstance
{
public:
    ~CInstance();

    void Create(IVulkanWindow* window);
    [[nodiscard]] vk::Instance GetHandle() const { return m_handle; }

private:
    vk::Instance m_handle;

    vk::DebugUtilsMessengerEXT m_debugMessenger;
};
}
