#pragma once

#include "../vulkan.hpp"
#include "../vulkan_window.hpp"

class CVulkanInstance
{
public:
    void Create(IVulkanWindow* window);
    [[nodiscard]] vk::Instance GetHandle() const { return m_instance; }

private:
    vk::Instance m_instance;
};
