#pragma once

#include "vulkan.hpp"
#include "vulkan_window.hpp"

namespace Vulkan
{
class CInstance
{
public:
    CInstance() = default;
    CInstance(const CInstance&) = default;
    CInstance(CInstance&&) = default;
    CInstance& operator=(const CInstance&) = default;
    CInstance& operator=(CInstance&&) = default;
    ~CInstance();

    void Create(const IVulkanWindow* window);
    [[nodiscard]] vk::Instance GetHandle() const { return m_handle; }

private:
    vk::Instance m_handle;

    vk::DebugUtilsMessengerEXT m_debugMessenger;
};
}
