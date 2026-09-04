#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
CContext::CContext(const IWindow* window, const IOSConnector* osConnector) :
    m_window(window),
    m_osConnector(osConnector),
    m_instance(osConnector),
    m_surface(m_instance, osConnector),
    m_physicalDevice(m_instance, m_surface),
    m_device(m_instance, m_physicalDevice),
    m_allocator(m_instance, *m_physicalDevice, m_device)
{

}

void CContext::RepairSurface() {
    m_surface = CSurface { m_instance, m_osConnector };
}
}
