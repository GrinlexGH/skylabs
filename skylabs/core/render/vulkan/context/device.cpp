#include <skylabs/core/render/vulkan/context/device.hpp>

#include <fmt/ranges.h>

namespace {
bool TryEnableFeatures(vkb::PhysicalDevice& phys, const vk::PhysicalDeviceFeatures& f) {
    return phys.enable_features_if_present(static_cast<VkPhysicalDeviceFeatures>(f));
}

template <typename T>
bool TryEnableFeatures(vkb::PhysicalDevice& phys, const T& f) {
    return phys.enable_extension_features_if_present(f);
}
}

namespace Vulkan {
CDevice::CDevice(
    const CInstance& instance,
    CPhysicalDevice& physicalDevice
) {
    auto& phys = physicalDevice.VkbPhysicalDevice();

    #define VK_OPT_FEATURE(x, y) \
        do { \
            x.y = vk::True; \
            m_caps.m_ ##y = TryEnableFeatures(phys, x); /* PHYSX REFERENCE */ \
        } while(false)

    vk::PhysicalDeviceFeatures features10 {};
    VK_OPT_FEATURE(features10, samplerAnisotropy);

    vk::PhysicalDeviceVulkan14Features features14 {};
    vk::PhysicalDeviceMaintenance5Features maintenance5 {};

    if (instance.ApiVersion() >= vk::ApiVersion14) {
        VK_OPT_FEATURE(features14, maintenance5);
    } else {
        if (phys.enable_extension_if_present(vk::KHRMaintenance5ExtensionName)) {
            VK_OPT_FEATURE(maintenance5, maintenance5);
        }
    }

    vkb::DeviceBuilder builder { phys };
    auto deviceResult = builder.build();
    if (!deviceResult) {
        throw std::runtime_error(
            fmt::format("Failed to create vulkan device ({}): {}, {}",
                vk::to_string(vk::Result { deviceResult.vk_result() }),
                deviceResult.error().message(),
                deviceResult.detailed_failure_reasons()
            )
        );
    }

    m_vkbDevice = deviceResult.value();

    m_handle = vk::raii::Device { *physicalDevice, m_vkbDevice.device };

    auto getQueue = [this](CQueue& queue, vkb::QueueType type) {
        auto result = m_vkbDevice.get_queue_and_index(type);
        if (!result) {
            throw std::runtime_error(
                fmt::format(fmt::runtime("Failed to get {} queue ({}): {}, {}"),
                    vk::to_string(vk::Result { result.vk_result() }),
                    result.error().message(),
                    result.detailed_failure_reasons()
                )
            );
        }

        auto [vkQueue, index] = *result;
        queue = CQueue { m_handle, vkQueue, index };
    };

    getQueue(m_graphicsQueue, vkb::QueueType::graphics);
    getQueue(m_presentQueue, vkb::QueueType::present);
    getQueue(m_computeQueue, vkb::QueueType::compute);

    for (auto& name : phys.get_extensions()) {
        m_enabledExtensions.emplace(name);
    }
}
}
