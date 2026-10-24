#include <flat_map>

#include <VkBootstrap.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "project_info.hpp"
#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/render/vulkan/context/context.hpp"

// Contract:
//  1. You choose target required Vulkan API version and write WHOLE code only for it
//  2. You choose real target device(s) to require features
//  3. Every feature that does not support your target device(s) is optional

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
            sk::log::Info(sk::log::Category::eVulkan, "{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            sk::log::Warning(sk::log::Category::eVulkan, "{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            sk::log::Error(sk::log::Category::eVulkan, "{}\n", pCallbackData->pMessage);
            break;
    }

    return vk::False;
}
#endif

enum class ExtensionRequirement : std::uint8_t { eOptional, eRequired };

sk::render::vulkan::ExtensionSet SetupInstanceExtensions(
    vkb::InstanceBuilder& instanceBuilder, const sk::render::vulkan::IOSAdapter* osAdapter) {
    auto systemInfoResult = vkb::SystemInfo::get_system_info(osAdapter->GetVkGetInstanceProcAddr());
    if (!systemInfoResult) {
        throw std::runtime_error { fmt::format("Failed to query vulkan system info: {}",
                                               systemInfoResult.error().message()) };
    }
    const vkb::SystemInfo& systemInfo = systemInfoResult.value();

    // Request extensions
    std::flat_map<std::string_view, ExtensionRequirement> requestedExtensions {
#ifdef DEBUG
        { vk::EXTDebugUtilsExtensionName, ExtensionRequirement::eOptional },
#endif
        { vk::KHRGetSurfaceCapabilities2ExtensionName, ExtensionRequirement::eOptional },
        { vk::EXTSwapchainColorSpaceExtensionName, ExtensionRequirement::eOptional },
    };

    for (auto& ext : osAdapter->RequiredInstanceExtensions()) {
        requestedExtensions[ext] = ExtensionRequirement::eRequired;
    }

    // Find these extensions
    std::vector<std::string> enabledExtensions;

    for (const auto& [nameView, requirement] : requestedExtensions) {
        // Intentionally unconditionally enable required extensions
        if (std::string name { nameView }; requirement == ExtensionRequirement::eRequired ||
                                           systemInfo.is_extension_available(name.c_str())) {
            enabledExtensions.emplace_back(std::move(name));
        }
    }

    instanceBuilder.enable_extensions(enabledExtensions);

    // Debug utils
#ifdef DEBUG
    if (systemInfo.debug_utils_available) {
        instanceBuilder
#ifndef ARCH_32
            .request_validation_layers()
#endif
            .enable_extension(vk::EXTDebugUtilsExtensionName)
            .set_debug_callback(reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(
                reinterpret_cast<std::uintptr_t>(DebugCallback)))
            .set_debug_messenger_severity(static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError))
            .set_debug_messenger_type(static_cast<VkDebugUtilsMessageTypeFlagsEXT>(
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance));
    }
#endif

    return sk::render::vulkan::ExtensionSet { enabledExtensions };
}

struct InstanceCreationResult {
    vkb::Instance instance;
    sk::render::vulkan::ExtensionSet enabledExtensions;
};

// See comment on top of this file, ChoosePhysicalDevice() and CreateDevice()
//
// !!! DO NOT FORGET TO UPDATE EXTENSION FEATURES TO CORE FEATURES WHEN UPDATING API VERSION
constexpr std::uint32_t kApiVersion = vk::ApiVersion13;

InstanceCreationResult CreateInstance(const sk::render::vulkan::IOSAdapter* osAdapter) {
    constexpr std::uint32_t appVersion = vk::makeApiVersion(
        0, project_info::kVersionMajor, project_info::kVersionMinor, project_info::kVersionPatch);

    vkb::InstanceBuilder instanceBuilder;
    instanceBuilder.set_app_name(project_info::kGameName)
        .set_app_version(appVersion)
        .set_engine_name(project_info::kName)
        .set_engine_version(appVersion)
        .require_api_version(kApiVersion);

    sk::render::vulkan::ExtensionSet enabledExtensions =
        SetupInstanceExtensions(instanceBuilder, osAdapter);

    auto instanceResult = instanceBuilder.build();
    if (!instanceResult) {
        throw std::runtime_error { fmt::format("Failed to create vulkan instance ({}): {}",
                                               instanceResult.error().message(),
                                               instanceResult.detailed_failure_reasons()) };
    }

    return InstanceCreationResult { .instance = instanceResult.value(),
                                    .enabledExtensions = std::move(enabledExtensions) };
}

vkb::PhysicalDevice ChoosePhysicalDevice(const vkb::Instance& instance, const vk::SurfaceKHR& surface) {
    vkb::PhysicalDeviceSelector selector { instance, surface };

    // !!! Required Vulkan features.
    //
    // Feature intersection of my own devices:
    //  Samsung A54
    //  Lenovo IdeaPad pro 5 (14imh9)
    //  RTX 3060 VISION OC
    //
    // Vulkan 1.3
    // Extensions:
    //  VK_KHR_swapchain
    // Features:
    //  samplerAnisotropy
    //  synchronization2
    //  dynamicRendering
    //  maintenance4
    //  bufferDeviceAddress
    //
    // !!! DO NOT FORGET TO UPDATE EXTENSION FEATURES TO CORE FEATURES WHEN UPDATING API VERSION

    // Required extensions
    selector.add_required_extension(vk::KHRSwapchainExtensionName);

    // Required features
    vk::PhysicalDeviceFeatures features10 { };
    features10.samplerAnisotropy = vk::True;
    selector.set_required_features(features10);

    vk::PhysicalDeviceVulkan12Features features12 { };
    features12.bufferDeviceAddress = vk::True;
    selector.set_required_features_12(features12);

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

enum class ExtensionAvailability : std::uint8_t { eAvailable, eUnavailable };

// Needs to control extension dependencies
class DeviceExtensions {
public:
    explicit DeviceExtensions(vkb::PhysicalDevice& pd) : m_pd(pd) { }

    bool Enable(const char* name) {
        auto [it, inserted] = m_state.try_emplace(name, ExtensionAvailability::eUnavailable);
        if (inserted)
            it->second = m_pd.enable_extension_if_present(name) ? ExtensionAvailability::eAvailable
                                                                : ExtensionAvailability::eUnavailable;
        return it->second == ExtensionAvailability::eAvailable;
    }

    bool Any(const std::initializer_list<const char*> names) {
        for (const char* n : names)
            if (Enable(n)) return true;
        return false;
    }

    bool All(const std::initializer_list<const char*> names) {
        for (const char* n : names)
            if (!m_pd.is_extension_present(n)) return false;
        for (const char* n : names) Enable(n);
        return true;
    }

private:
    vkb::PhysicalDevice& m_pd;
    std::flat_map<std::string, ExtensionAvailability> m_state;
};

template <typename T>
bool TryEnableFeatures(vkb::PhysicalDevice& physicalDevice, const T& f) {
    return physicalDevice.enable_extension_features_if_present(f);
}

sk::render::vulkan::Device CreateDevice(vkb::PhysicalDevice& physicalDevice,
                                        sk::render::vulkan::PhysicalDevice&& raiiPhysicalDevice) {
    // !!! Optional Vulkan features.
    //
    // Features:
    //  maintenance5
    //  extendedFlags
    //  descriptorHeap
    //
    // !!! DO NOT FORGET TO UPDATE EXTENSION FEATURES TO CORE FEATURES WHEN UPDATING API VERSION

    sk::render::vulkan::DeviceCaps caps;
    caps.apiVersion = kApiVersion;

#define VK_OPT_FEATURE(x, y)                           \
    do {                                               \
        (x).y = vk::True;                              \
        caps.y = TryEnableFeatures(physicalDevice, x); \
    } while (false)

    DeviceExtensions exts { physicalDevice };

    if (exts.Enable(vk::KHRMaintenance5ExtensionName)) {
        vk::PhysicalDeviceMaintenance5Features maintenance5 { };
        VK_OPT_FEATURE(maintenance5, maintenance5);
    }

    if (exts.Enable(vk::KHRExtendedFlagsExtensionName)) {
        vk::PhysicalDeviceExtendedFlagsFeaturesKHR extendedFlags { };
        VK_OPT_FEATURE(extendedFlags, extendedFlags);
    }

    if (exts.Any({ vk::KHRMaintenance5ExtensionName, vk::KHRExtendedFlagsExtensionName }) &&
        exts.Enable(vk::EXTDescriptorHeapExtensionName)) {
        vk::PhysicalDeviceDescriptorHeapFeaturesEXT heap { };
        VK_OPT_FEATURE(heap, descriptorHeap);
    }

    if (exts.Enable(vk::EXTMemoryPriorityExtensionName)) {
        vk::PhysicalDeviceMemoryPriorityFeaturesEXT memoryPriority { };
        VK_OPT_FEATURE(memoryPriority, memoryPriority);

        // Don't care, just try to enable
        if (exts.Enable(vk::EXTPageableDeviceLocalMemoryExtensionName)) {
            vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageable { vk::True };
            TryEnableFeatures(physicalDevice, pageable);
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
    auto [vkbInstance, enabledExtensions] = CreateInstance(osAdapter);
    m_instance = Instance { { osAdapter->GetVkGetInstanceProcAddr() },
                            vkbInstance.instance,
                            vkbInstance.debug_messenger,
                            std::move(enabledExtensions) };

    // Build surface
    m_surface = Surface { m_instance, osAdapter };

    // Choose physical device
    vkb::PhysicalDevice physicalDevice = ChoosePhysicalDevice(vkbInstance, *m_surface);

    // Build device
    m_device = CreateDevice(physicalDevice, PhysicalDevice { *m_instance, physicalDevice.physical_device,
                                                             physicalDevice.properties.deviceName });

    // Build allocator
    m_allocator = Allocator { *m_instance, m_device };
}

void Context::RecreateSurface() { m_surface = Surface { m_instance, m_osAdapter }; }
}
