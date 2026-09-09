#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/public/logging.hpp>
#include "project_info.hpp"

namespace {
#ifdef DEBUG
VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/
) {
    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            Log::Info(Log::Category::eVulkan, "{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            Log::Warning(Log::Category::eVulkan, "{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            Log::Error(Log::Category::eVulkan, "{}\n", pCallbackData->pMessage);
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

std::vector<std::string> SetupInstanceExtensions(const vk::raii::Context& context,
                                                 const Vulkan::IOSConnector* osConnector,
                                                 [[maybe_unused]] const bool setupDebugUtils) {
    std::unordered_map<std::string_view, bool> requestedExtensions {
        { vk::EXTSwapchainColorSpaceExtensionName, false }
    };

#ifdef DEBUG
    if (setupDebugUtils) {
        requestedExtensions.try_emplace(vk::EXTDebugUtilsExtensionName, false);
    }
#endif

    for (auto& ext : osConnector->RequiredInstanceExtensions()) {
        requestedExtensions.try_emplace(ext, true);
    }

    if (requestedExtensions.empty()) {
        return { };
    }

    // Find these extensions
    std::vector<std::string_view> missingExtensions;
    std::vector<std::string> enabledExtensions;
    enabledExtensions.reserve(requestedExtensions.size());
    for (const auto& extension : GetAvailableExtensions(context)) {
        if (const std::string_view name { extension.extensionName };
            requestedExtensions.contains(name)) {
            enabledExtensions.emplace_back(name);
        } else if (requestedExtensions[name]) {
            missingExtensions.push_back(name);
        }
    }

    if (!missingExtensions.empty()) {
        throw std::runtime_error { fmt::format(
            "Missing required vulkan instance extensions: {}",
            fmt::join(missingExtensions.begin(), missingExtensions.end(), ", ")) };
    }

    return enabledExtensions;
}

struct InstanceCreationResult {
    vk::raii::Context context;
    vkb::Instance instance;
    std::vector<std::string> enabledExtensions;
};

InstanceCreationResult CreateInstance(const Vulkan::IOSConnector* osConnector,
                                      const bool setupDebugUtils = true) {
    vk::raii::Context context { osConnector->GetVkGetInstanceProcAddr() };

    std::vector<std::string> enabledExtensions =
        SetupInstanceExtensions(context, osConnector, setupDebugUtils);

    constexpr std::uint32_t appVersion =
        vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);

    std::vector<const char*> rawEnabledExtensions { };
    rawEnabledExtensions.reserve(enabledExtensions.size());
    for (const auto& ext : enabledExtensions) {
        rawEnabledExtensions.push_back(ext.c_str());
    }

    vkb::InstanceBuilder instanceBuilder;
    instanceBuilder.set_app_name(Skylabs::GAME_NAME)
        .set_app_version(appVersion)
        .set_engine_name(Skylabs::NAME)
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

    // !!! Required vulkan features

    // Minimum version
    selector.set_minimum_version(1, 3);

    // Required extensions
    selector.add_required_extension(vk::KHRSwapchainExtensionName);

    // Required extension features
    vk::PhysicalDeviceDescriptorIndexingFeatures descIndexing { };
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
                        fmt::join(physicalDeviceResult.detailed_failure_reasons(), "; ")));
    }

    return physicalDeviceResult.value();
}

bool TryEnableFeatures(vkb::PhysicalDevice& physicalDevice, const vk::PhysicalDeviceFeatures& f) {
    return physicalDevice.enable_features_if_present(f);
}

template <typename T>
bool TryEnableFeatures(vkb::PhysicalDevice& physicalDevice, const T& f) {
    return physicalDevice.enable_extension_features_if_present(f);
}

Vulkan::CDevice CreateDevice(vkb::PhysicalDevice& physicalDevice,
                             Vulkan::CPhysicalDevice&& raiiPhysicalDevice) {
    // !!! Optional vulkan features
    Vulkan::DeviceCaps caps;

#define VK_OPT_FEATURE(x, y)                           \
    do {                                               \
        (x).y = vk::True;                              \
        caps.y = TryEnableFeatures(physicalDevice, x); \
    } while (false)

    vk::PhysicalDeviceFeatures features10 { };
    VK_OPT_FEATURE(features10, samplerAnisotropy);

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

    auto getQueue = [&](Vulkan::CQueue& queue, const vkb::QueueType type) {
        auto result = deviceResult.value().get_queue_and_index(type);
        if (!result) {
            throw std::runtime_error(fmt::format(fmt::runtime("Failed to get {} queue ({}): {}, {}"),
                                                 vk::to_string(vk::Result { result.vk_result() }),
                                                 result.error().message(),
                                                 result.detailed_failure_reasons()));
        }

        auto [vkQueue, index] = *result;
        queue = Vulkan::CQueue { device, vkQueue, index };
    };

    Vulkan::CQueue graphicsQueue { nullptr };
    getQueue(graphicsQueue, vkb::QueueType::graphics);

    Vulkan::CQueue presentQueue { nullptr };
    getQueue(presentQueue, vkb::QueueType::present);

    Vulkan::CQueue computeQueue { nullptr };
    getQueue(computeQueue, vkb::QueueType::compute);

    return Vulkan::CDevice {
        std::move(device),        std::move(raiiPhysicalDevice), physicalDevice.get_extensions(), caps,
        std::move(graphicsQueue), std::move(presentQueue),       std::move(computeQueue)
    };
}
}

namespace Vulkan {
CContext::CContext(const IWindow* window, const IOSConnector* osConnector)
    : m_window(window), m_osConnector(osConnector) {
    // Build instance
    auto [context, vkbInstance, enabledExtensions] = CreateInstance(osConnector);
    m_instance = CInstance { context, vkbInstance.instance, vkbInstance.debug_messenger,
                             std::move(enabledExtensions) };

    // Build surface
    m_surface = CSurface { m_instance, osConnector };

    // Choose physical device
    vkb::PhysicalDevice physicalDevice = ChoosePhysicalDevice(vkbInstance, *m_surface);

    // Build device
    m_device =
        CreateDevice(physicalDevice, CPhysicalDevice { *m_instance, physicalDevice.physical_device,
                                                       physicalDevice.properties.deviceName,
                                                       physicalDevice.properties.apiVersion });

    // Build allocator
    m_allocator = CAllocator { *m_instance, m_device };
}

void CContext::RepairSurface() { m_surface = CSurface { m_instance, m_osConnector }; }
}
