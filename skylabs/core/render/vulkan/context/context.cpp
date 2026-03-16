#include <skylabs/core/render/vulkan/context/context.hpp>

#include <ranges>

namespace Vulkan {
CContext::CContext(const IWindow* const window) : m_window(window)
{
    bool initialized = false;
    for (const auto& profile : { CProfile::Profile::eRoadmap2024, CProfile::Profile::eRoadmap2022 }) {
        m_profile = CProfile { profile };

        if (!m_profile.CheckInstanceSupport())
            continue;

        m_instance = CInstance { m_profile, m_window->GetRequiredInstanceExtensions() };

        std::optional physicalDevice = SelectPhysicalDevice();
        if (!physicalDevice.has_value())
            continue;

        m_physicalDevice = physicalDevice.value();
        m_device = CDevice { m_profile, m_window, m_instance, m_physicalDevice };
        m_allocator = CAllocator { m_profile, *m_instance, *m_physicalDevice, m_device };
        initialized = true;
        break;
    }

    if (!initialized) {
        throw std::runtime_error("Failed to initialize vulkan context");
    }
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

    if (!m_profile.CheckPhysicalDeviceSupport(**m_instance, *physicalDevice))
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
}
