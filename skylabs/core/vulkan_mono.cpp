#include <skylabs/core/vulkan_mono.hpp>

#include <skylabs/core/SDL/context.hpp>
#include <skylabs/core/SDL/vulkan/window.hpp>
#include <skylabs/public/logging.hpp>
#include "project_info.hpp"

#include <vulkan/vulkan_raii.hpp>
#include <fmt/ranges.h>
#include <vk_mem_alloc_raii.hpp>

#include <utility>
#include <ranges>
#include <set>

class CTimer
{
public:
    CTimer() {
        Log::Debug("Timer start");
        m_start = std::chrono::steady_clock::now();
    }
    CTimer(const CTimer&) = delete;
    CTimer(CTimer&&) = delete;
    CTimer& operator=(const CTimer&) = delete;
    CTimer& operator=(CTimer&&) = delete;
    ~CTimer() {
        const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        Log::Debug("{} ms", std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count());
    }

private:
    std::chrono::steady_clock::time_point m_start;
};

void AppendToPNextChain(void*& currentChain, void* newExtension) {
    if (currentChain == nullptr) {
        currentChain = newExtension;
        return;
    }

    auto* current = static_cast<vk::BaseOutStructure*>(currentChain);
    while (current->pNext != nullptr) {
        current = current->pNext;
    }

    current->pNext = static_cast<vk::BaseOutStructure*>(newExtension);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/
) {
    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            Log::Info("{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            Log::Warning("{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            Log::Error("{}", pCallbackData->pMessage);
            break;
    }

    return vk::False;
}

namespace VulkanStructured {
enum class Requirement : std::uint8_t { eOptional, eRequired };

struct CRequestedExtension
{
    std::string_view m_name;
    Requirement m_requirement;
};

// region Instance
class CInstance
{
public:
    CInstance() = delete;
    explicit CInstance(std::nullptr_t) {}
    explicit CInstance(
        const vk::raii::Context& context,
        std::uint32_t apiVersion,
        std::span<CRequestedExtension> requestedExtensions
    );
    CInstance(CInstance&) = delete;
    CInstance(CInstance&&) = default;
    CInstance& operator=(CInstance&) = delete;
    CInstance& operator=(CInstance&&) = default;
    ~CInstance() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::Instance& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::raii::Instance* { return &m_handle; }

    [[nodiscard]] auto ApiVersion() const noexcept -> std::uint32_t { return m_apiVersion; }
    [[nodiscard]] auto IsExtensionEnabled(const std::string_view name) const -> bool { return std::ranges::contains(m_activeExtensions, name); }

private:
    vk::raii::Instance m_handle { nullptr };

    std::vector<std::string> m_activeExtensions;
    std::uint32_t m_apiVersion = 0;

#ifdef DEBUG
    vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger { nullptr };
#endif
};

CInstance::CInstance(
    const vk::raii::Context& context,
    const std::uint32_t apiVersion,
    const std::span<CRequestedExtension> requestedExtensions
) {
    // Collect all available global extensions
    std::vector<vk::ExtensionProperties> globalAvailableExtensions = context.enumerateInstanceExtensionProperties();

    // Enable validation layer
    std::vector<const char*> enabledLayers { };
#ifdef DEBUG
    {
        const std::vector<vk::LayerProperties> availableLayers = context.enumerateInstanceLayerProperties();

        constexpr auto validationLayer = "VK_LAYER_KHRONOS_validation";
        const bool hasValidation = std::ranges::any_of(availableLayers, [&](const auto& l) {
            return std::string_view(l.layerName) == validationLayer;
        });

        if (hasValidation) {
            Log::Debug("Enabling {}", validationLayer);
            enabledLayers.push_back(validationLayer);

            // If VK_LAYER_KHRONOS_validation is enabled, extensions from it are also available
            std::vector<vk::ExtensionProperties> validationLayerExtensions =
                context.enumerateInstanceExtensionProperties(std::string { validationLayer });
            globalAvailableExtensions.insert(
                globalAvailableExtensions.end(),
                validationLayerExtensions.begin(), validationLayerExtensions.end()
            );
        }
    }
#endif

    const auto isExtensionAvailable = [&](const std::string_view name) {
        return std::ranges::any_of(globalAvailableExtensions, [&](const vk::ExtensionProperties& ext) {
            return ext.extensionName == name;
        });
    };

    // Enable extensions
    void* pNext = nullptr;

#ifdef DEBUG
    // Debug utils messenger is the only object that need to be created in instance
    bool isDebugUtilsAvailable = false;
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo {};
    if (isExtensionAvailable(vk::EXTDebugUtilsExtensionName)) {
        isDebugUtilsAvailable = true;

        m_activeExtensions.emplace_back(vk::EXTDebugUtilsExtensionName);
        debugUtilsCreateInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        debugUtilsCreateInfo.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        debugUtilsCreateInfo.pfnUserCallback = DebugCallback;

        AppendToPNextChain(pNext, &debugUtilsCreateInfo);
    }
#endif

    // Process requested extensions
    std::vector<std::string_view> missingExtensions { };
    for (const auto& [name, requirement] : requestedExtensions) {
        if (isExtensionAvailable(name)) {
            m_activeExtensions.emplace_back(name);
        } else {
            if (requirement == Requirement::eRequired) {
                missingExtensions.push_back(name);
            } else {
                Log::Debug("Optional extension {} is not supported", name);
            }
        }
    }

    // Some required extensions are missing...
    if (!missingExtensions.empty()) {
        throw std::runtime_error(fmt::format(
            "System doesn't have required instance extensions:\n    {}",
            fmt::join(missingExtensions, "\n    ")
        ));
    }

    // For instance creation
    std::vector<const char*> enabledExtensions { };
    enabledExtensions.reserve(m_activeExtensions.size());
    for (const auto& ext : m_activeExtensions) {
        enabledExtensions.push_back(ext.c_str());
    }

    vk::ApplicationInfo applicationInfo {};
    applicationInfo.pApplicationName = Skylabs::GAME_NAME;
    applicationInfo.applicationVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    applicationInfo.pEngineName = Skylabs::NAME;
    applicationInfo.engineVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    applicationInfo.apiVersion = m_apiVersion = apiVersion;

    vk::InstanceCreateInfo instanceCreateInfo {};
    instanceCreateInfo.pNext = pNext;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = !enabledLayers.empty() ? enabledLayers.data() : nullptr;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = !enabledExtensions.empty() ? enabledExtensions.data() : nullptr;

    m_handle = vk::raii::Instance { context, instanceCreateInfo };
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_handle);

#ifdef DEBUG
    if (isDebugUtilsAvailable) {
        debugUtilsMessenger = vk::raii::DebugUtilsMessengerEXT { m_handle, debugUtilsCreateInfo };
    }
#endif
}
// endregion

class CGPUInfo
{
public:
    explicit CGPUInfo(std::nullptr_t) {}
    explicit CGPUInfo(vk::raii::PhysicalDevice physicalDevice) :
        m_handle(std::move(physicalDevice)),
        m_queueFamilies(m_handle.getQueueFamilyProperties2KHR()),
        m_availableExtensions(m_handle.enumerateDeviceExtensionProperties()),
        m_properties(m_handle.getProperties2KHR()),
        m_features(
            m_handle.getFeatures2KHR<
                vk::PhysicalDeviceFeatures2KHR,
                vk::PhysicalDeviceVulkan11Features,
                vk::PhysicalDeviceVulkan12Features,
                vk::PhysicalDeviceVulkan13Features,
                vk::PhysicalDeviceDynamicRenderingFeaturesKHR
            >()
        )
    {}

    [[nodiscard]] auto Handle() const noexcept -> const vk::raii::PhysicalDevice& { return m_handle; }
    [[nodiscard]] auto QueueFamilies() const noexcept -> const std::vector<vk::QueueFamilyProperties2KHR>& { return m_queueFamilies; }
    [[nodiscard]] auto AvailableExtensions() const noexcept -> const std::vector<vk::ExtensionProperties>& { return m_availableExtensions; }
    [[nodiscard]] auto Properties() const noexcept -> const vk::PhysicalDeviceProperties2KHR& { return m_properties; }
    [[nodiscard]] auto Features() const noexcept -> const auto& { return m_features; }

    [[nodiscard]] auto IsExtensionAvailable(const std::string_view name) const noexcept -> bool {
        return std::ranges::any_of(
            m_availableExtensions,
            [&](const vk::ExtensionProperties& ext) { return name == ext.extensionName; }
        );
    }

private:
    vk::raii::PhysicalDevice m_handle { nullptr };
    std::vector<vk::QueueFamilyProperties2KHR> m_queueFamilies;
    std::vector<vk::ExtensionProperties> m_availableExtensions;
    vk::PhysicalDeviceProperties2KHR m_properties;
    vk::StructureChain<
        vk::PhysicalDeviceFeatures2KHR,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceDynamicRenderingFeaturesKHR
    > m_features;
};

class CDevice
{
public:
    using DeviceFeatures = vk::StructureChain<
        vk::PhysicalDeviceFeatures2KHR,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceDynamicRenderingFeaturesKHR
    >;

    struct CRequestedFeature
    {
        using PFN_enable = bool(*)(
            std::uint32_t apiVersion,
            const CGPUInfo& gpu,
            DeviceFeatures& features,
            std::vector<const char*>& deviceExtensions
        );

        PFN_enable m_enable;
        Requirement m_requirement;
    };

    explicit CDevice(std::nullptr_t) {}
    explicit CDevice(
        const ::Vulkan::IWindow* window,
        vk::Instance instance,
        const CGPUInfo& gpu,
        std::uint32_t apiVersion,
        std::span<CRequestedFeature> requestedFeatures
    );
    CDevice(CDevice&) = delete;
    CDevice(CDevice&&) = default;
    CDevice& operator=(CDevice&) = delete;
    CDevice& operator=(CDevice&&) = default;
    ~CDevice() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::Device& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::raii::Device* { return &m_handle; }

    [[nodiscard]] auto ApiVersion() const noexcept -> std::uint32_t { return m_apiVersion; }
    [[nodiscard]] auto IsExtensionEnabled(const std::string_view name) const -> bool { return std::ranges::contains(m_activeExtensions, name); }

    template <typename Feature>
    [[nodiscard]] auto IsFeatureEnabled(vk::Bool32 Feature::* flag) const -> Feature {
        return m_enabledFeatures.get<Feature>().*flag;
    }

private:
    [[nodiscard]] static auto GetQueueCreateInfos(
        const ::Vulkan::IWindow* window,
        vk::Instance instance,
        const CGPUInfo& gpu
    ) -> std::vector<vk::DeviceQueueCreateInfo>;

    vk::raii::Device m_handle { nullptr };

    std::uint32_t m_apiVersion = 0;
    std::vector<std::string> m_activeExtensions;
    DeviceFeatures m_enabledFeatures;
};

std::vector<vk::DeviceQueueCreateInfo> CDevice::GetQueueCreateInfos(
    const ::Vulkan::IWindow* window,
    const vk::Instance instance,
    const CGPUInfo& gpu
) {
    std::optional<std::uint32_t> graphicsFamily;
    std::optional<std::uint32_t> presentFamily;
    std::optional<std::uint32_t> computeFamily;
    const auto& queueFamilies = gpu.QueueFamilies();

    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (!graphicsFamily.has_value() && queueFamilies[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsFamily.emplace(i);
        }

        // Graphics queue guarantees compute queue (but not on the same physical device...)
        if (!computeFamily.has_value() && queueFamilies[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eCompute) {
            computeFamily.emplace(i);
        }

        if (!presentFamily.has_value() && window->IsQueueFamilySupportPresent(instance, gpu.Handle(), i)) {
            presentFamily.emplace(i);
        }

        if (graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value()) {
            break;
        }
    }

    std::array uniqueQueueFamilies { *graphicsFamily, *presentFamily, *computeFamily };
    std::ranges::sort(uniqueQueueFamilies);
    const std::size_t uniqueCount =
        std::distance(uniqueQueueFamilies.begin(), std::ranges::unique(uniqueQueueFamilies).begin());

    static constexpr float queuePriority = 0.5f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueCount);
    for (std::size_t i = 0; i < uniqueCount; ++i) {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies.at(i);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    return queueCreateInfos;
}

CDevice::CDevice(
    const ::Vulkan::IWindow* window,
    const vk::Instance instance,
    const CGPUInfo& gpu,
    const std::uint32_t apiVersion,
    std::span<CRequestedFeature> requestedFeatures
) : m_apiVersion(apiVersion) {
    const std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = GetQueueCreateInfos(window, instance, gpu);

    DeviceFeatures finalFeatures;

    if (apiVersion < vk::ApiVersion13) {
        finalFeatures.unlink<vk::PhysicalDeviceVulkan13Features>();

        if (apiVersion < vk::ApiVersion12) {
            finalFeatures.unlink<vk::PhysicalDeviceVulkan12Features>();

            if (apiVersion < vk::ApiVersion11) {
                finalFeatures.unlink<vk::PhysicalDeviceVulkan11Features>();
            }
        }
    }

    std::vector<const char*> enabledExtensions { };
    for (const auto& [enable, requirement] : requestedFeatures) {
        if (!enable(apiVersion, gpu, finalFeatures, enabledExtensions)) {
            if (requirement == Requirement::eRequired) {
                throw std::runtime_error("System can't enable required feature! See logs");
            }
            Log::Debug("Can't enable optional feature. See logs");
        }
    }

    // Remove duplicates
    std::ranges::sort(enabledExtensions);
    enabledExtensions.erase(enabledExtensions.end(), std::ranges::unique(enabledExtensions).end());

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = !queueCreateInfos.empty() ? queueCreateInfos.data() : nullptr;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = !enabledExtensions.empty() ? enabledExtensions.data() : nullptr;
    deviceCreateInfo.pNext = (m_enabledFeatures = finalFeatures).get<vk::PhysicalDeviceFeatures2KHR>();

    auto* current = static_cast<const vk::BaseInStructure*>(deviceCreateInfo.pNext);
    while (current) {
        Log::Debug("{}", vk::to_string(current->sType));
        current = current->pNext;
    }

    m_handle = vk::raii::Device { gpu.Handle(), deviceCreateInfo };
}

class CContext
{
public:
    CContext() = delete;
    explicit CContext(std::nullptr_t) {}
    explicit CContext(const ::Vulkan::IWindow* window);
    CContext(CContext&) = delete;
    CContext(CContext&&) = default;
    CContext& operator=(CContext&) = delete;
    CContext& operator=(CContext&&) = default;
    ~CContext() = default;

    [[nodiscard]] auto Instance() const noexcept -> const CInstance& { return m_instance; }
    [[nodiscard]] auto Window() const noexcept -> const Vulkan::IWindow* { return m_window; }

private:
    static auto EnableDynamicRender(
        std::uint32_t apiVersion,
        const CGPUInfo& gpu,
        CDevice::DeviceFeatures& features,
        std::vector<const char*>& deviceExtensions
    ) -> bool;

    auto CreateInstance() -> void;
    [[nodiscard]] auto SelectPhysicalDevice(const vk::raii::PhysicalDevices& physicalDevices) const -> CGPUInfo;
    [[nodiscard]] auto IsDeviceSuitable(const CGPUInfo& physicalDeviceInfo) const -> bool;
    [[nodiscard]] auto RatePhysicalDevice(const CGPUInfo& physicalDeviceInfo) const -> int;
    auto CreateDevice() -> void;

    const Vulkan::IWindow* m_window = nullptr;
    vk::raii::Context m_context;
    CInstance m_instance { nullptr };
    vk::raii::PhysicalDevice m_physicalDevice { nullptr };
    CDevice m_device { nullptr };
};

CContext::CContext(const ::Vulkan::IWindow* const window) : m_window(window) {
    CreateInstance();
    CreateDevice();
}

void CContext::CreateInstance() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    std::uint32_t apiVersion = vk::ApiVersion10;
    if (m_context.getDispatcher()->vkEnumerateInstanceVersion) {
        apiVersion = m_context.enumerateInstanceVersion();
        Log::Debug("Available Vulkan version: {}.{}.{}, but anyways I will use Vulkan 1.0",
            vk::apiVersionMajor(apiVersion), vk::apiVersionMinor(apiVersion), vk::apiVersionPatch(apiVersion)
        );

        apiVersion = vk::ApiVersion10;
    }

    std::vector<CRequestedExtension> instanceExtensions = m_window->GetRequiredInstanceExtensions()
        | std::views::transform([](const char* const ext) -> CRequestedExtension { return { .m_name = ext, .m_requirement = Requirement::eRequired }; })
        | std::ranges::to<std::vector<CRequestedExtension>>();

    if (apiVersion < vk::ApiVersion11) {
        instanceExtensions.emplace_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName, Requirement::eRequired);
    }

    m_instance = CInstance { m_context, apiVersion, instanceExtensions };
}

// Guarantees:
// * Graphics & present queue families
// * VK_KHR_swapchain extension
// * SamplerAnisotropy feature
bool CContext::IsDeviceSuitable(const CGPUInfo& physicalDeviceInfo) const {
    // Check for graphics and queue families
    bool hasGraphicsQueue = false;
    bool hasPresentQueue = false;

    const std::vector<vk::QueueFamilyProperties2KHR>& queueFamilies = physicalDeviceInfo.QueueFamilies();
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies.at(i).queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)
            hasGraphicsQueue = true;
        if (m_window->IsQueueFamilySupportPresent(*m_instance, *physicalDeviceInfo.Handle(), i))
            hasPresentQueue = true;
        if (hasGraphicsQueue && hasPresentQueue) break;
    }

    if (!hasGraphicsQueue || !hasPresentQueue)
        return false;

    // Check for swapchain extension
    bool hasSwapchainExtension = false;

    for (const auto& extension : physicalDeviceInfo.AvailableExtensions()) {
        if (std::strcmp(extension.extensionName, vk::KHRSwapchainExtensionName) == 0) {
            hasSwapchainExtension = true;
            break;
        }
    }

    if (!hasSwapchainExtension)
        return false;

    // Check for sampler anisotropy
    if (!physicalDeviceInfo.Features().get<vk::PhysicalDeviceFeatures2KHR>().features.samplerAnisotropy)
        return false;

    return true;
}

int CContext::RatePhysicalDevice(const CGPUInfo& physicalDeviceInfo) const {
    int score = 0;

    if (!IsDeviceSuitable(physicalDeviceInfo))
        return score;

    switch (physicalDeviceInfo.Properties().properties.deviceType) {
        case vk::PhysicalDeviceType::eDiscreteGpu: score += 2000; break;
        case vk::PhysicalDeviceType::eIntegratedGpu: score += 800; break;
        case vk::PhysicalDeviceType::eVirtualGpu: score += 500; break;
        case vk::PhysicalDeviceType::eCpu: score += 200; break;
        case vk::PhysicalDeviceType::eOther: score += 100; break;
    }

    return score;
}

CGPUInfo CContext::SelectPhysicalDevice(const vk::raii::PhysicalDevices& physicalDevices) const {
    CGPUInfo deviceInfo { nullptr };
    int maxScore = 0;

    for (const auto& device : physicalDevices) {
        CGPUInfo currentDeviceInfo { device };
        if (const int score = RatePhysicalDevice(currentDeviceInfo); score > maxScore) {
            maxScore = score;
            deviceInfo = std::move(currentDeviceInfo);
        }
    }

    if (maxScore == 0) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    return deviceInfo;
}

void CContext::CreateDevice() {
    const vk::raii::PhysicalDevices physicalDevices { *m_instance };
    const CGPUInfo selectedGPU = SelectPhysicalDevice(physicalDevices);

    const std::uint32_t deviceApiVersion = selectedGPU.Properties().properties.apiVersion;
    const std::uint32_t usingApiVersion = std::min(m_instance.ApiVersion(), deviceApiVersion);
    Log::Debug("Device vulkan api version is {}.{}.{}, minimum version is {}.{}.{}",
        vk::apiVersionMajor(deviceApiVersion), vk::apiVersionMinor(deviceApiVersion), vk::apiVersionPatch(deviceApiVersion),
        vk::apiVersionMajor(usingApiVersion), vk::apiVersionMinor(usingApiVersion), vk::apiVersionPatch(usingApiVersion)
    );

    std::vector<CDevice::CRequestedFeature> deviceFeatures;
    deviceFeatures.emplace_back(EnableDynamicRender, Requirement::eOptional);

    m_device = CDevice { m_window, *m_instance, selectedGPU, usingApiVersion, deviceFeatures };
}

bool CContext::EnableDynamicRender(
    const std::uint32_t apiVersion,
    const CGPUInfo& gpu,
    CDevice::DeviceFeatures& features,
    std::vector<const char*>& deviceExtensions
) {
    if (apiVersion >= vk::ApiVersion13) {
        features.unlink<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>();
        features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = vk::True;
        return true;
    }

    if (gpu.IsExtensionAvailable(vk::KHRDynamicRenderingExtensionName) &&
        gpu.Features().get<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>().dynamicRendering
    ) {
        features.get<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>().dynamicRendering = vk::True;
        deviceExtensions.emplace_back(vk::KHRDynamicRenderingExtensionName);

        // dependencies
        if (apiVersion < vk::ApiVersion12) {
            deviceExtensions.emplace_back(vk::KHRDepthStencilResolveExtensionName);
            deviceExtensions.emplace_back(vk::KHRCreateRenderpass2ExtensionName);
            if (apiVersion < vk::ApiVersion11) {
                deviceExtensions.emplace_back(vk::KHRMultiviewExtensionName);
                deviceExtensions.emplace_back(vk::KHRMaintenance2ExtensionName);
            }
        }

        return true;
    }

    features.unlink<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>();
    return false;
}
}

namespace Vulkan {
void CVulkanMono::CreateWindow() {
    m_SDLContext = SDL::CContext { SDL_INIT_VIDEO };
    m_SDLWindow = SDL::Vulkan::CWindow { "Skylabs", 640, 480, SDL_WINDOW_RESIZABLE };
    SDL_SetWindowRelativeMouseMode(*m_SDLWindow, true);
}

void CVulkanMono::Run() {
    CreateWindow();

    CTimer timer;
    using namespace VulkanStructured;

    CContext m_context = CContext { &m_SDLWindow };
}
}
