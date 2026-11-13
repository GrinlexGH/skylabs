#include <flat_map>

#include <VkBootstrap.h>

#include "project_info.hpp"
#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/render/vulkan/context.hpp"

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

template <typename R>
auto Unwrap(const R&& result, const std::string_view what) {
    if (!result) {
        const auto& reasons = result.detailed_failure_reasons();
        if (reasons.empty()) {
            throw std::runtime_error { fmt::format("{}: {}", what, result.error().message()) };
        }

        throw std::runtime_error { fmt::format("{} ({}):\n{}", what, result.error().message(),
                                               fmt::join(reasons, "\n")) };
    }
    return std::forward<const R>(result).value();
}

enum class ExtensionRequirement : std::uint8_t { eOptional, eRequired };

// !!! DO NOT FORGET TO UPDATE EXTENSION FEATURES TO CORE FEATURES WHEN UPDATING API VERSION
constexpr std::uint32_t kApiVersion = vk::ApiVersion13;

vkb::Instance CreateInstance(const sk::render::vulkan::IOSAdapter* osAdapter) {
    // !!! Required Vulkan features.
    //
    // Feature intersection of my own devices:
    //  Samsung A54
    //  Lenovo IdeaPad pro 5 (14imh9)
    //  RTX 3060 VISION OC
    //
    // Vulkan 1.3
    // Extensions:
    //  VK_KHR_get_surface_capabilities2
    //  VK_KHR_swapchain
    //
    // !!! DO NOT FORGET TO UPDATE EXTENSION FEATURES TO CORE FEATURES WHEN UPDATING API VERSION

    constexpr std::uint32_t appVersion = vk::makeApiVersion(
        0, project_info::kVersionMajor, project_info::kVersionMinor, project_info::kVersionPatch);

    vkb::InstanceBuilder instanceBuilder;
    instanceBuilder.set_app_name(project_info::kGameName)
        .set_app_version(appVersion)
        .set_engine_name(project_info::kName)
        .set_engine_version(appVersion)
        .require_api_version(kApiVersion);

    const vkb::SystemInfo systemInfo =
        Unwrap(vkb::SystemInfo::get_system_info(osAdapter->GetVkGetInstanceProcAddr()),
               "Failed to query vulkan system info");

    // Request extensions
    std::flat_map<std::string_view, ExtensionRequirement> requestedExtensions {
#ifdef DEBUG
        { vk::EXTDebugUtilsExtensionName, ExtensionRequirement::eOptional },
#endif
        { vk::KHRGetSurfaceCapabilities2ExtensionName, ExtensionRequirement::eRequired },
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

    return Unwrap(instanceBuilder.build(), "Failed to create vulkan instance");
}

vkb::PhysicalDevice ChoosePhysicalDevice(const vkb::Instance& instance, const vk::SurfaceKHR& surface) {
    vkb::PhysicalDeviceSelector selector { instance, surface };

    // !!! Required Vulkan features.
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

    return Unwrap(selector.select(), "Failed to select vulkan physical device");
}

struct DeviceResult {
    vkb::Device device;
    sk::render::vulkan::DeviceCaps caps;
};

DeviceResult CreateDevice(vkb::PhysicalDevice& physicalDevice) {
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

    if (physicalDevice.enable_extension_if_present(vk::KHRMaintenance5ExtensionName)) {
        vk::PhysicalDeviceMaintenance5Features maintenance5 { };
        maintenance5.maintenance5 = vk::True;
        caps.maintenance5 = physicalDevice.enable_extension_features_if_present(maintenance5);
    }

    if (physicalDevice.enable_extension_if_present(vk::KHRExtendedFlagsExtensionName)) {
        vk::PhysicalDeviceExtendedFlagsFeaturesKHR extendedFlags { };
        extendedFlags.extendedFlags = vk::True;
        caps.extendedFlags = physicalDevice.enable_extension_features_if_present(extendedFlags);
    }

    if (physicalDevice.enable_extension_if_present(vk::EXTDescriptorHeapExtensionName)) {
        vk::PhysicalDeviceDescriptorHeapFeaturesEXT descriptorHeap { };
        descriptorHeap.descriptorHeap = vk::True;
        caps.extendedFlags = physicalDevice.enable_extension_features_if_present(descriptorHeap);
    }

    if (physicalDevice.enable_extension_if_present(vk::EXTMemoryPriorityExtensionName)) {
        vk::PhysicalDeviceMemoryPriorityFeaturesEXT memoryPriority { };
        memoryPriority.memoryPriority = vk::True;
        caps.memoryPriority = physicalDevice.enable_extension_features_if_present(memoryPriority);

        // Don't care, just try to enable
        if (physicalDevice.enable_extension_if_present(vk::EXTPageableDeviceLocalMemoryExtensionName)) {
            vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageable { vk::True };
            physicalDevice.enable_extension_features_if_present(pageable);
        }
    }

    caps.descriptorHeap = false;
    vkb::DeviceBuilder builder { physicalDevice };
    return { .device = Unwrap(builder.build(), "Failed to create device"), .caps = caps };
}

sk::render::vulkan::Queues QueryQueues(const vkb::Device& device, const vk::raii::Device& raiiDevice) {
    auto getQueue = [&](const vkb::QueueType type) {
        auto [vkQueue, index] = Unwrap(device.get_queue_and_index(type), "Failed to get vulkan queue");
        return sk::render::vulkan::Queue { raiiDevice, vkQueue, index };
    };

    return sk::render::vulkan::Queues { .compute = getQueue(vkb::QueueType::compute),
                                        .graphics = getQueue(vkb::QueueType::graphics),
                                        .present = getQueue(vkb::QueueType::present) };
}

vma::raii::Allocator CreateAllocator(const vk::raii::Instance& instance,
                                     sk::render::vulkan::Device& device,
                                     const vk::PhysicalDevice physicalDevice) {
    vma::AllocatorCreateFlags flags = vma::AllocatorCreateFlagBits::eKhrMaintenance4 |
                                      vma::AllocatorCreateFlagBits::eBufferDeviceAddress;

    if (device.Caps().maintenance5) {
        flags |= vma::AllocatorCreateFlagBits::eKhrMaintenance5;
    }

    if (device.Caps().memoryPriority) {
        flags |= vma::AllocatorCreateFlagBits::eExtMemoryPriority;
    }

    vma::AllocatorCreateInfo allocatorCreateInfo;
    allocatorCreateInfo.flags = flags;
    allocatorCreateInfo.vulkanApiVersion = device.Caps().apiVersion;
    allocatorCreateInfo.physicalDevice = physicalDevice;

    return vma::raii::Allocator { instance, *device, allocatorCreateInfo };
}
}

namespace sk::render::vulkan {
struct Context::InitData {
    const IWindow* window;
    const IOSAdapter* osAdapter;
    vk::raii::Instance instance;
    vk::raii::DebugUtilsMessengerEXT debugMessenger;
    vk::raii::SurfaceKHR surface;
    vk::raii::PhysicalDevice physicalDevice;
    Device device;
    Queues queues;
    vma::raii::Allocator allocator;
};

Context::Context(const IWindow* window, const IOSAdapter* osAdapter)
    : Context { Initialize(window, osAdapter) } { }

Context::InitData Context::Initialize(const IWindow* window, const IOSAdapter* osAdapter) {
    vkb::Instance vkbInstance = CreateInstance(osAdapter);

    vk::raii::Context vkContext { osAdapter->GetVkGetInstanceProcAddr() };
    vk::raii::Instance instance { vkContext, vkbInstance.instance };

    vk::raii::DebugUtilsMessengerEXT messenger { nullptr };
    if (vkbInstance.debug_messenger != VK_NULL_HANDLE) {
        messenger = vk::raii::DebugUtilsMessengerEXT { instance, vkbInstance.debug_messenger };
    }

    vk::raii::SurfaceKHR surface =
        vk::raii::SurfaceKHR { instance, osAdapter->CreateSurface(*instance) };

    vkb::PhysicalDevice vkbPhysicalDevice = ChoosePhysicalDevice(vkbInstance, *surface);
    vk::raii::PhysicalDevice physicalDevice { instance, vkbPhysicalDevice.physical_device };

    auto [vkbDevice, caps] = CreateDevice(vkbPhysicalDevice);
    caps.debugUtils = messenger != nullptr;
    Device device { vk::raii::Device { physicalDevice, vkbDevice.device }, caps };

    Queues queues = QueryQueues(vkbDevice, *device);
    vma::raii::Allocator allocator = CreateAllocator(instance, device, physicalDevice);

    return InitData { .window = window,
                      .osAdapter = osAdapter,
                      .instance = std::move(instance),
                      .debugMessenger = std::move(messenger),
                      .surface = std::move(surface),
                      .physicalDevice = std::move(physicalDevice),
                      .device = std::move(device),
                      .queues = std::move(queues),
                      .allocator = std::move(allocator) };
}

Context::Context(InitData&& initData)
    : m_window(initData.window),
      m_osAdapter(initData.osAdapter),
      m_instance(std::move(initData.instance)),
      m_debugUtilsMessenger(std::move(initData.debugMessenger)),
      m_surface(std::move(initData.surface)),
      m_physicalDevice(std::move(initData.physicalDevice)),
      m_device(std::move(initData.device)),
      m_queues(std::move(initData.queues)),
      m_allocator(std::move(initData.allocator)) { }

void Context::RecreateSurface() {
    m_surface = vk::raii::SurfaceKHR { m_instance, m_osAdapter->CreateSurface(*m_instance) };
}
}
