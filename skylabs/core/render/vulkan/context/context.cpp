#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/public/logging.hpp>

#include <ranges>

namespace Vulkan {
CContext::CContext(const IWindow* const window) : m_window(window) {
    m_instance = CInstance { { m_window->GetRequiredInstanceExtensions() } };

    std::optional physicalDevice = SelectPhysicalDevice();
    if (!physicalDevice.has_value()) {
        throw std::runtime_error("No suitable vulkan device available");
    }

    m_physicalDevice = *physicalDevice;
    m_device = CDevice { m_window, m_instance, m_physicalDevice };
    m_allocator = CAllocator { m_instance, *m_physicalDevice, m_device };
}

std::optional<CPhysicalDevice> CContext::SelectPhysicalDevice() {
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
        return std::nullopt;
    }

    return selectedGPU;
}

int CContext::RatePhysicalDevice(const CPhysicalDevice& physicalDevice) const {
    int score = 0;

    vk::PhysicalDeviceProperties deviceProperties = physicalDevice->getProperties2().properties;
    if (deviceProperties.apiVersion < m_instance.ApiVersion())
        return score;

    switch (deviceProperties.deviceType) {
        case vk::PhysicalDeviceType::eDiscreteGpu: score += 2000; break;
        case vk::PhysicalDeviceType::eIntegratedGpu: score += 800; break;
        case vk::PhysicalDeviceType::eVirtualGpu: score += 500; break;
        case vk::PhysicalDeviceType::eCpu: score += 200; break;
        case vk::PhysicalDeviceType::eOther: score += 100; break;
    }

    return score;
}
}
