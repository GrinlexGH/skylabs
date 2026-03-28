#include <skylabs/core/render/vulkan/context/physical_device.hpp>

#include <fmt/ranges.h>

namespace Vulkan {
CPhysicalDevice::CPhysicalDevice(const CInstance& instance, const CSurface& surface) {
    vkb::PhysicalDeviceSelector selector { instance.VkbInstance(), **surface };
    auto physicalDeviceResult = selector.select();
    if (!physicalDeviceResult) {
        throw std::runtime_error(
            fmt::format("Failed to select vulkan physical device ({}): {}, {}",
                vk::to_string(vk::Result { physicalDeviceResult.vk_result() }),
                physicalDeviceResult.error().message(),
                fmt::join(physicalDeviceResult.detailed_failure_reasons(), "; ")
            )
        );
    }

    m_vkbPhysicalDevice = physicalDeviceResult.value();
    m_handle = vk::raii::PhysicalDevice { *instance, m_vkbPhysicalDevice };
}
}
