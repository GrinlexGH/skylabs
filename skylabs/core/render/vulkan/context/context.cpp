#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/profile.hpp>
#include <skylabs/public/logging.hpp>

#include <ranges>

namespace Vulkan {
CContext::CContext(const IWindow* const window) :
    m_window(window),
    m_instance(m_window->GetRequiredInstanceExtensions()),
    m_device(m_window, m_instance, SelectPhysicalDevice()),
    m_allocator(*m_instance, *m_physicalDevice, m_device)
{}

bool CContext::IsDeviceSuitable(const CPhysicalDevice& physicalDevice) const {
    return Profile::CheckPhysicalDeviceSupport(**m_instance, **physicalDevice);
}

int CContext::RatePhysicalDevice(const CPhysicalDevice& physicalDevice) const {
    int score = 0;

    if (!IsDeviceSuitable(physicalDevice))
        return score;

    switch (physicalDevice->getProperties2().properties.deviceType) {
        case vk::PhysicalDeviceType::eDiscreteGpu: score += 2000; break;
        case vk::PhysicalDeviceType::eIntegratedGpu: score += 800; break;
        case vk::PhysicalDeviceType::eVirtualGpu: score += 500; break;
        case vk::PhysicalDeviceType::eCpu: score += 200; break;
        case vk::PhysicalDeviceType::eOther: score += 100; break;
    }

    return score;
}

CPhysicalDevice CContext::SelectPhysicalDevice() {
    CPhysicalDevice selectedGPU { nullptr };
    int maxScore = 0;

    for (const auto& device : vk::raii::PhysicalDevices { *m_instance }) {
        CPhysicalDevice physicalDevice { device };
        if (const int score = RatePhysicalDevice(physicalDevice); score > maxScore) {
            maxScore = score;
            selectedGPU = std::move(physicalDevice);
        }
    }

    if (maxScore == 0) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    m_physicalDevice = selectedGPU;

    return selectedGPU;
}
}
