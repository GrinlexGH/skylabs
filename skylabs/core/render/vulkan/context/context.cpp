#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
CContext::CContext(const IWindow* const window) :
    m_window(window),
    m_instance(),
    m_surface(m_instance, window),
    m_physicalDevice(m_instance, m_surface),
    m_device(m_instance, m_physicalDevice),
    m_allocator(m_instance, *m_physicalDevice, m_device)
{}

void CContext::RepairSurface() {
    m_surface = CSurface { m_instance, m_window };
}
}
