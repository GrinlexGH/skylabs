#include <skylabs/core/vulkan_mono.hpp>

#include <skylabs/core/SDL/context.hpp>
#include <skylabs/core/SDL/vulkan/window.hpp>
#include <skylabs/public/logging.hpp>
#include "project_info.hpp"

#include <vulkan/vulkan_raii.hpp>
#include <fmt/ranges.h>

#include <ranges>
#include <flat_set>

// Fix defines
#ifdef CreateWindow
#undef CreateWindow
#endif

class CTimer
{
public:
    CTimer() {
        Log::Debug("Timer start");
        m_start = std::chrono::steady_clock::now();
    }

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
struct CRequestedExtension
{
    enum class Requirement : std::uint8_t { eOptional, eRequired };

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

    [[nodiscard]] auto IsExtensionEnabled(const std::string_view name) const -> bool { return m_activeExtensions.contains(name); }

private:
    vk::raii::Instance m_handle { nullptr };

    std::flat_set<std::string, std::less<>> m_activeExtensions;

#ifdef DEBUG
    vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger { nullptr };
#endif
};

CInstance::CInstance(
    const vk::raii::Context& context,
    const std::uint32_t apiVersion,
    const std::span<CRequestedExtension> requestedExtensions
) {
    // Validate api version
    if (const std::uint32_t availableApiVer = context.enumerateInstanceVersion(); availableApiVer < apiVersion) {
        throw std::runtime_error(fmt::format(
            "Requested API {}.{}.{} is not supported. Available: {}.{}.{}",
            vk::apiVersionMajor(apiVersion), vk::apiVersionMinor(apiVersion), vk::apiVersionPatch(apiVersion),
            vk::apiVersionMajor(availableApiVer), vk::apiVersionMinor(availableApiVer), vk::apiVersionPatch(availableApiVer)
        ));
    }

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

    const std::flat_set<std::string_view> availableExtensions = globalAvailableExtensions
        | std::views::transform([](const vk::ExtensionProperties& ext) -> std::string_view { return ext.extensionName; })
        | std::ranges::to<std::flat_set<std::string_view>>();

    const auto isExtensionAvailable = [&](const std::string_view name) {
        return availableExtensions.contains(name);
    };

    // Enable extensions
    void* pNext = nullptr;

#ifdef DEBUG
    // Debug utils messenger is the only object that need to be created in instance
    bool isDebugUtilsAvailable = false;
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo {};
    if (isExtensionAvailable(vk::EXTDebugUtilsExtensionName)) {
        isDebugUtilsAvailable = true;

        m_activeExtensions.emplace(vk::EXTDebugUtilsExtensionName);
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
    std::flat_set<std::string_view> missingExtensions { };
    for (const auto& [name, requirement] : requestedExtensions) {
        if (isExtensionAvailable(name)) {
            m_activeExtensions.emplace(name);
        } else {
            if (requirement == CRequestedExtension::Requirement::eRequired) [[unlikely]] {
                missingExtensions.emplace(name);
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
    applicationInfo.apiVersion = vk::ApiVersion10;

    vk::InstanceCreateInfo instanceCreateInfo {};
    instanceCreateInfo.pNext = pNext;
    instanceCreateInfo.flags = {};
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
    auto CreateInstance() -> void;
    auto SelectPhysicalDevice() -> void;
    auto IsDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) const -> bool;
    auto RatePhysicalDevice(const vk::raii::PhysicalDevice& physicalDevice) const -> int;

    const Vulkan::IWindow* m_window { nullptr };
    vk::raii::Context m_context;
    CInstance m_instance { nullptr };
    vk::raii::PhysicalDevice m_physicalDevice { nullptr };
};

// Guarantees:
// * Graphics & present queue families
// * VK_KHR_swapchain extension
// * SamplerAnisotropy feature
bool CContext::IsDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) const {
    // Check for graphics and queue families
    const std::vector<vk::QueueFamilyProperties2KHR> queueFamilies = physicalDevice.getQueueFamilyProperties2KHR();
    bool hasGraphicsQueue = false;
    bool hasPresentQueue = false;

    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)
            hasGraphicsQueue = true;
        if (m_window->IsQueueFamilySupportPresent(*m_instance, *physicalDevice, i))
            hasPresentQueue = true;
        if (hasGraphicsQueue && hasPresentQueue) break;
    }

    if (!hasGraphicsQueue || !hasPresentQueue)
        return false;

    // Check for swapchain extension
    bool hasSwapchainExtension = false;

    for (const auto& extension : physicalDevice.enumerateDeviceExtensionProperties()) {
        if (std::strcmp(extension.extensionName, vk::KHRSwapchainExtensionName) == 0) {
            hasSwapchainExtension = true;
            break;
        }
    }

    if (!hasSwapchainExtension)
        return false;

    // Check for sampler anisotropy
    if (!physicalDevice.getFeatures2KHR().features.samplerAnisotropy)
        return false;

    return true;
}

int CContext::RatePhysicalDevice(const vk::raii::PhysicalDevice& physicalDevice) const {
    int score = 0;

    if (!IsDeviceSuitable(physicalDevice))
        return score;

    switch (physicalDevice.getProperties2KHR().properties.deviceType) {
        case vk::PhysicalDeviceType::eDiscreteGpu: score += 2000; break;
        case vk::PhysicalDeviceType::eIntegratedGpu: score += 800; break;
        case vk::PhysicalDeviceType::eVirtualGpu: score += 500; break;
        case vk::PhysicalDeviceType::eCpu: score += 200; break;
        case vk::PhysicalDeviceType::eOther: score += 100; break;
    }

    return score;
}

CContext::CContext(const ::Vulkan::IWindow* const window) : m_window(window) {
    CreateInstance();
    SelectPhysicalDevice();
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

    using Req = CRequestedExtension::Requirement;
    std::vector<CRequestedExtension> instanceExtensions = m_window->GetRequiredInstanceExtensions()
        | std::views::transform([](const char* const ext) -> CRequestedExtension { return { .m_name = ext, .m_requirement = Req::eRequired }; })
        | std::ranges::to<std::vector<CRequestedExtension>>();

    if (apiVersion < vk::ApiVersion11) {
        instanceExtensions.emplace_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName, Req::eRequired);
    }

    m_instance = CInstance { m_context, apiVersion, instanceExtensions };
}

void CContext::SelectPhysicalDevice() {
    // TODO: wrapper with all extensions and properties
    const vk::raii::PhysicalDevices physicalDevices { *m_instance };

    int maxScore = -1;

    for (const auto& device : physicalDevices) {
        if (const int score = RatePhysicalDevice(device); score > maxScore) {
            maxScore = score;
            m_physicalDevice = device;
        }
    }

    if (maxScore <= 0) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    auto props = m_physicalDevice.getProperties2KHR();
    Log::Info("Selected GPU: {}", std::string_view{ props.properties.deviceName });
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
