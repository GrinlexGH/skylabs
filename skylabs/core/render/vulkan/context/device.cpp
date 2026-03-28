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

    #define VK_REQ_EXT(x) \
        do { \
            if (!phys.enable_extension_if_present(x)) { \
                throw std::runtime_error(#x " device extension is required but not available"); \
            } \
        } while(false)

    #define VK_REQ_FEATURE(x, y) \
        do { \
            x.y = vk::True; \
            if (!TryEnableFeatures(phys, x)) { \
                throw std::runtime_error(#y " device feature is required but not available"); \
            } \
        } while(false)

    #define VK_OPT_FEATURE(x, y) \
        do { \
            x.y = vk::True; \
            m_caps.m_ ##y = TryEnableFeatures(phys, x); \
        } while(false)

    VK_REQ_EXT(vk::KHRSwapchainExtensionName);

    vk::PhysicalDeviceFeatures features {};
    VK_REQ_FEATURE(features, samplerAnisotropy);

    vk::PhysicalDeviceVulkan11Features features11 {};
    VK_REQ_FEATURE(features11, shaderDrawParameters);

    vk::PhysicalDeviceVulkan13Features features13 {};
    VK_REQ_FEATURE(features13, synchronization2);
    VK_REQ_FEATURE(features13, dynamicRendering);
    VK_REQ_FEATURE(features13, maintenance4);

    vk::PhysicalDeviceVulkan14Features features14 {};
    vk::PhysicalDeviceMaintenance5Features maintenance5 {};

    if (instance.ApiVersion() >= vk::ApiVersion14) {
        VK_OPT_FEATURE(features14, maintenance5);
    } else {
        if (phys.enable_extension_if_present(vk::KHRMaintenance5ExtensionName)) {
            m_enabledExtensions.emplace(vk::KHRMaintenance5ExtensionName);
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
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_handle);

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
