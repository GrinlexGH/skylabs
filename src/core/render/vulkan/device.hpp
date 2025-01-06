#pragma once

#include "vulkan.hpp"

namespace Vulkan
{
class CDevice
{
public:
    void Create(vk::PhysicalDevice physicalDevice, const std::vector<const char*>& requiredExtensions);

private:
    vk::Device m_handle;
};
}
