#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/context/profile.hpp>
#include <skylabs/public/logging.hpp>

#include <ranges>

namespace Vulkan {
CContext::CContext(const IWindow* const window) :
    m_window(window),
    m_profile(CProfile::Profiles::eRoadmap2022),
    m_instance(m_profile, m_window->GetRequiredInstanceExtensions()),
    m_device(m_profile, m_window, m_instance, SelectPhysicalDevice()),
    m_allocator(m_profile, *m_instance, *m_physicalDevice, m_device)
{}

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

bool CContext::IsDeviceSuitable(const CPhysicalDevice& physicalDevice) const {
    auto getFeatures = [&]<typename T>() {
        if (physicalDevice->getDispatcher()->vkGetPhysicalDeviceFeatures2) {
            return physicalDevice->getFeatures2<vk::PhysicalDeviceFeatures2, T>().template get<T>();
        }
        if (physicalDevice->getDispatcher()->vkGetPhysicalDeviceFeatures2KHR) {
            return physicalDevice->getFeatures2KHR<vk::PhysicalDeviceFeatures2, T>().template get<T>();
        }
        return T {};
    };

    const auto features11 = getFeatures.operator()<vk::PhysicalDeviceVulkan11Features>();
    bool hasDrawParameters = features11.shaderDrawParameters == vk::True;

    return hasDrawParameters && m_profile.CheckPhysicalDeviceSupport(**m_instance, **physicalDevice);
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
}
