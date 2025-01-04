#pragma once
#include "vulkan.hpp"

namespace Vulkan
{
class CPhysicalDevice
{
public:
    void Pick(vk::Instance instance, const std::vector<const char*>& requiredExtensions = {});

private:
    vk::PhysicalDevice m_handle;
};
}
