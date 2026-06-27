module;
#include <VkBootstrap.h>
module skylabs.vulkan.context;
import :physical_device;
import fmt;

namespace Vulkan {
CPhysicalDevice::CPhysicalDevice(const CInstance& instance, const CSurface& surface) {
    vkb::PhysicalDeviceSelector selector { instance.VkbInstance(), **surface };

    // Minimum version
    selector.set_minimum_version(1, 3);

    // Required extensions
    selector.add_required_extension(vk::KHRSwapchainExtensionName);

    // Required extension features
    vk::PhysicalDeviceDescriptorIndexingFeatures descIndexing {};
    descIndexing.setDescriptorBindingPartiallyBound(vk::True);
    descIndexing.setRuntimeDescriptorArray(vk::True);
    descIndexing.setDescriptorBindingSampledImageUpdateAfterBind(vk::True);

    selector.add_required_extension_features(descIndexing);

    // Vulkan 1.1 features
    vk::PhysicalDeviceVulkan11Features features11 { };
    features11.shaderDrawParameters = vk::True;
    selector.set_required_features_11(features11);

    // Vulkan 1.3 features
    vk::PhysicalDeviceVulkan13Features features13 { };
    features13.synchronization2 = vk::True;
    features13.dynamicRendering = vk::True;
    features13.maintenance4 = vk::True;
    selector.set_required_features_13(features13);

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
