#include <flat_map>

#include <VkBootstrap.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "project_info.hpp"
#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/render/vulkan/context/context.hpp"

namespace {
#ifdef DEBUG
VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/
) {
    namespace sk_log = sk::log;

    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            sk_log::Info(sk_log::Category::eVulkan, "{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            sk_log::Warning(sk_log::Category::eVulkan, "{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            sk_log::Error(sk_log::Category::eVulkan, "{}\n", pCallbackData->pMessage);
            break;
    }

    return vk::False;
}
#endif

std::vector<vk::ExtensionProperties> GetAvailableExtensions(const vk::raii::Context& context) {
    std::vector<vk::ExtensionProperties> globalExtensions =
        context.enumerateInstanceExtensionProperties();

#if defined(DEBUG) && !defined(ARCH_32)
    for (auto& layer : context.enumerateInstanceLayerProperties()) {
        if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") != 0) {
            continue;
        }

        const std::vector<vk::ExtensionProperties> layerExtensions =
            context.enumerateInstanceExtensionProperties({ layer.layerName.data() });

        globalExtensions.reserve(globalExtensions.size() + layerExtensions.size());
        for (auto& ext : layerExtensions) {
            globalExtensions.emplace_back(ext.extensionName);
        }

        break;
    }
#endif

    return globalExtensions;
}

enum class ExtensionRequirement : std::uint8_t { eOptional, eRequired };

sk::render::vulkan::ExtensionSet SetupInstanceExtensions(const vk::raii::Context& context,
                                                         const sk::render::vulkan::IOSAdapter* osAdapter,
                                                         [[maybe_unused]] const bool setupDebugUtils) {
    std::flat_map<std::string_view, ExtensionRequirement> requestedExtensions {
        { vk::KHRGetSurfaceCapabilities2ExtensionName, ExtensionRequirement::eOptional },
        { vk::EXTSwapchainColorSpaceExtensionName, ExtensionRequirement::eOptional },
    };

#ifdef DEBUG
    if (setupDebugUtils) {
        requestedExtensions[vk::EXTDebugUtilsExtensionName] = ExtensionRequirement::eOptional;
    }
#endif

    for (auto& ext : osAdapter->RequiredInstanceExtensions()) {
        requestedExtensions[ext] = ExtensionRequirement::eRequired;
    }

    // Find these extensions
    std::vector<std::string> enabledExtensions;
    enabledExtensions.reserve(requestedExtensions.size());
    for (const auto& extension : GetAvailableExtensions(context)) {
        if (const std::string_view name { extension.extensionName };
            requestedExtensions.contains(name)) {
            enabledExtensions.emplace_back(name);
        }
    }

    sk::render::vulkan::ExtensionSet enabledSet { enabledExtensions };

    std::vector<std::string_view> missingExtensions;
    for (const auto& [name, required] : requestedExtensions) {
        if (required == ExtensionRequirement::eRequired && !enabledSet.contains(name)) {
            missingExtensions.push_back(name);
        }
    }

    if (!missingExtensions.empty()) {
        throw std::runtime_error { fmt::format(
            "Missing required vulkan instance extensions: {}",
            fmt::join(missingExtensions.begin(), missingExtensions.end(), ", ")) };
    }

    return enabledSet;
}

struct InstanceCreationResult {
    vk::raii::Context context;
    vkb::Instance instance;
    sk::render::vulkan::ExtensionSet enabledExtensions;
};

InstanceCreationResult CreateInstance(const sk::render::vulkan::IOSAdapter* osAdapter,
                                      const bool setupDebugUtils = true) {
    vk::raii::Context context { osAdapter->GetVkGetInstanceProcAddr() };

    sk::render::vulkan::ExtensionSet enabledExtensions =
        SetupInstanceExtensions(context, osAdapter, setupDebugUtils);

    constexpr std::uint32_t appVersion = vk::makeApiVersion(
        0, project_info::kVersionMajor, project_info::kVersionMinor, project_info::kVersionPatch);

    std::vector<const char*> rawEnabledExtensions { };
    rawEnabledExtensions.reserve(enabledExtensions.size());
    for (const auto& ext : enabledExtensions) {
        rawEnabledExtensions.push_back(ext.c_str());
    }

    vkb::InstanceBuilder instanceBuilder;
    instanceBuilder.set_app_name(project_info::kGameName)
        .set_app_version(appVersion)
        .set_engine_name(project_info::kName)
        .set_engine_version(appVersion)
        .set_minimum_instance_version(vk::ApiVersion13)
        .enable_extensions(rawEnabledExtensions);

#ifdef DEBUG
    constexpr auto debugSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                   vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                                   vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                   vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    constexpr auto debugTypes = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

    if (setupDebugUtils && std::ranges::contains(enabledExtensions, vk::EXTDebugUtilsExtensionName)) {
        instanceBuilder
#ifndef ARCH_32
            .request_validation_layers()
#endif
            .set_debug_callback(reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(
                reinterpret_cast<std::uintptr_t>(DebugCallback)))
            .set_debug_messenger_severity(
                static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(debugSeverity))
            .set_debug_messenger_type(static_cast<VkDebugUtilsMessageTypeFlagsEXT>(debugTypes));
    }
#endif

    auto instanceResult = instanceBuilder.build();
    if (!instanceResult) {
        throw std::runtime_error { fmt::format("Failed to create vulkan instance ({}): {}, {}",
                                               vk::to_string(vk::Result { instanceResult.vk_result() }),
                                               instanceResult.error().message(),
                                               instanceResult.detailed_failure_reasons()) };
    }

    return InstanceCreationResult { .context = std::move(context),
                                    .instance = instanceResult.value(),
                                    .enabledExtensions = std::move(enabledExtensions) };
}

vkb::PhysicalDevice ChoosePhysicalDevice(const vkb::Instance& instance, const vk::SurfaceKHR& surface) {
    vkb::PhysicalDeviceSelector selector { instance, surface };

    // !!! Required Vulkan features

    // Feature intersection of my own devices:
    //  Lenovo IdeaPad pro 5 (14imh9)
    //  Samsung A54
    //  RTX 3060 VISION OC

    // Vulkan 1.3
    // Extensions:
    //  VK_KHR_swapchain
    // Features:
    //  samplerAnisotropy
    //  synchronization2
    //  dynamicRendering
    //  maintenance4

    // Minimum version
    selector.set_minimum_version(1, 3);

    // Required extensions
    selector.add_required_extension(vk::KHRSwapchainExtensionName);

    // Required features
    vk::PhysicalDeviceFeatures features10 { };
    features10.samplerAnisotropy = vk::True;
    selector.set_required_features(features10);

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
                        fmt::join(physicalDeviceResult.detailed_failure_reasons(), "; ")));
    }

    return physicalDeviceResult.value();
}

template <typename T>
bool TryEnableFeatures(vkb::PhysicalDevice& physicalDevice, const T& f) {
    return physicalDevice.enable_extension_features_if_present(f);
}

sk::render::vulkan::Device CreateDevice(vkb::PhysicalDevice& physicalDevice,
                                        sk::render::vulkan::PhysicalDevice&& raiiPhysicalDevice) {
    // !!! Optional Vulkan features

    // Features:
    //  maintenance5

    sk::render::vulkan::DeviceCaps caps;

#define VK_OPT_FEATURE(x, y)                           \
    do {                                               \
        (x).y = vk::True;                              \
        caps.y = TryEnableFeatures(physicalDevice, x); \
    } while (false)

    vk::PhysicalDeviceVulkan14Features features14 { };
    vk::PhysicalDeviceMaintenance5Features maintenance5 { };

    if (physicalDevice.properties.apiVersion >= vk::ApiVersion14) {
        VK_OPT_FEATURE(features14, maintenance5);
    } else {
        if (physicalDevice.enable_extension_if_present(vk::KHRMaintenance5ExtensionName)) {
            VK_OPT_FEATURE(maintenance5, maintenance5);
        }
    }

    vkb::DeviceBuilder builder { physicalDevice };
    auto deviceResult = builder.build();
    if (!deviceResult) {
        throw std::runtime_error(fmt::format("Failed to create vulkan device ({}): {}, {}",
                                             vk::to_string(vk::Result { deviceResult.vk_result() }),
                                             deviceResult.error().message(),
                                             deviceResult.detailed_failure_reasons()));
    }

    vk::raii::Device device { *raiiPhysicalDevice, deviceResult->device };

    auto getQueue = [&](sk::render::vulkan::Queue& queue, const vkb::QueueType type) {
        auto result = deviceResult.value().get_queue_and_index(type);
        if (!result) {
            throw std::runtime_error(fmt::format(fmt::runtime("Failed to get {} queue ({}): {}, {}"),
                                                 vk::to_string(vk::Result { result.vk_result() }),
                                                 result.error().message(),
                                                 result.detailed_failure_reasons()));
        }

        auto [vkQueue, index] = *result;
        queue = sk::render::vulkan::Queue { device, vkQueue, index };
    };

    sk::render::vulkan::Queue graphicsQueue { nullptr };
    getQueue(graphicsQueue, vkb::QueueType::graphics);

    sk::render::vulkan::Queue presentQueue { nullptr };
    getQueue(presentQueue, vkb::QueueType::present);

    sk::render::vulkan::Queue computeQueue { nullptr };
    getQueue(computeQueue, vkb::QueueType::compute);

    sk::render::vulkan::ExtensionSet extensions { physicalDevice.get_extensions() };

    return sk::render::vulkan::Device { std::move(device),        std::move(raiiPhysicalDevice),
                                        std::move(extensions),    caps,
                                        std::move(graphicsQueue), std::move(presentQueue),
                                        std::move(computeQueue) };
}
}

namespace sk::render::vulkan {
Context::Context(const IWindow* window, const IOSAdapter* osAdapter)
    : m_window(window), m_osAdapter(osAdapter) {
    // Build instance
    auto [context, vkbInstance, enabledExtensions] = CreateInstance(osAdapter);
    m_instance = Instance { context, vkbInstance.instance, vkbInstance.debug_messenger,
                            std::move(enabledExtensions) };

    // Build surface
    m_surface = Surface { m_instance, osAdapter };

    // Choose physical device
    vkb::PhysicalDevice physicalDevice = ChoosePhysicalDevice(vkbInstance, *m_surface);

    // Build device
    m_device = CreateDevice(physicalDevice, PhysicalDevice { *m_instance, physicalDevice.physical_device,
                                                             physicalDevice.properties.deviceName,
                                                             physicalDevice.properties.apiVersion });

    // Build allocator
    m_allocator = Allocator { *m_instance, m_device };
}

void Context::RecreateSurface() { m_surface = Surface { m_instance, m_osAdapter }; }
}
