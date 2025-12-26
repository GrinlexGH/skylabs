#pragma once
#include <skylabs/core/render/vulkan/context/extensions.hpp>
#include <skylabs/public/logging.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <unordered_map>
#include <unordered_set>

namespace Vulkan {
class CPhysicalDevice
{
public:
    explicit CPhysicalDevice(vk::raii::PhysicalDevice&& physicalDevice);
    CPhysicalDevice(const CPhysicalDevice&) = delete;
    CPhysicalDevice(CPhysicalDevice&&) noexcept = default;
    CPhysicalDevice& operator=(const CPhysicalDevice&) = delete;
    CPhysicalDevice& operator=(CPhysicalDevice&&) noexcept = default;
    ~CPhysicalDevice() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::PhysicalDevice& { return m_handle; }
    [[nodiscard]] auto GetHandle() const noexcept -> const vk::raii::PhysicalDevice& { return m_handle; }

    [[nodiscard]] auto GetProperties() const -> const vk::PhysicalDeviceProperties& { return m_properties; }
    [[nodiscard]] auto GetFeatures() const -> const vk::PhysicalDeviceFeatures& { return m_features; }
    [[nodiscard]] auto GetExtensions() const -> const std::vector<vk::ExtensionProperties>& { return m_extensions; }
    [[nodiscard]] auto GetQueueFamilies() const -> const std::vector<vk::QueueFamilyProperties>& { return m_queueFamilies; }

    [[nodiscard]] auto IsExtensionSupported(const std::string_view name) const -> bool { return m_availableExtensionsSet.contains(name); }

    [[nodiscard]] auto GetRequiredFeatures() const -> const vk::PhysicalDeviceFeatures& { return m_requiredFeatures; }
    [[nodiscard]] auto GetExtensionFeaturePNext() const -> void* { return m_extensionFeaturePNext; }

    template <typename Feature>
    [[nodiscard]] auto GetExtensionFeatures() const -> Feature {
        // VK_KHR_get_physical_device_properties2 is required
        return m_handle.getFeatures2KHR<vk::PhysicalDeviceFeatures2KHR, Feature>().template get<Feature>();
    }

    template <typename Feature>
    [[nodiscard]] auto IsExtensionFeatureEnabled(vk::Bool32 Feature::* flag) const -> vk::Bool32 {
        auto it = m_extensionFeatures.find(Feature::structureType);
        if (it != m_extensionFeatures.end() && it->second) {
            auto feature = std::static_pointer_cast<Feature>(it->second);
            return (*feature).*flag;
        }
        return vk::False;
    }

    template <typename Feature>
    auto AddExtensionFeatures() -> Feature& {
        auto [it, added] = m_extensionFeatures.try_emplace(Feature::structureType, std::make_shared<Feature>());
        if (added) {
            AppendToPNextChain(m_extensionFeaturePNext, it->second.get());
        }

        return *static_cast<Feature*>(it->second.get());
    }

    template <typename Feature>
    auto RequestRequiredExtensionFeature(vk::Bool32 Feature::* flag, std::string_view featureName, std::string_view flagName) -> void {
        if (GetExtensionFeatures<Feature>().*flag) {
            AddExtensionFeatures<Feature>().*flag = vk::True;
        } else {
            throw std::runtime_error(fmt::format("Requested required feature <{}::{}> is not supported!", featureName, flagName));
        }
    }

    template <typename Feature>
    auto RequestOptionalExtensionFeature(vk::Bool32 Feature::* flag, std::string_view featureName, std::string_view flagName) -> vk::Bool32 {
        const vk::Bool32 supported = GetExtensionFeatures<Feature>().*flag;
        if (supported) {
            AddExtensionFeatures<Feature>().*flag = vk::True;
        } else {
            Log::Info("Requested optional feature <{}::{}> is not supported", featureName, flagName);
        }

        return supported;
    }

    auto RequestRequiredFeature(vk::Bool32 vk::PhysicalDeviceFeatures::* flag, std::string_view flagName) -> void {
        if (m_features.*flag) {
            m_requiredFeatures.*flag = vk::True;
        } else {
            throw std::runtime_error(fmt::format("Requested required feature \"{}\" is not supported!", flagName));
        }
    }

    auto RequestOptionalFeature(vk::Bool32 vk::PhysicalDeviceFeatures::* flag, std::string_view flagName) -> vk::Bool32 {
        const vk::Bool32 supported = m_features.*flag;
        if (supported) {
            m_requiredFeatures.*flag = vk::True;
        } else {
            Log::Info("Requested optional feature \"{}\" is not supported.", flagName);
        }

        return supported;
    }

private:
    vk::raii::PhysicalDevice m_handle;

    // Properties
    vk::PhysicalDeviceProperties m_properties;
    vk::PhysicalDeviceFeatures m_features;
    std::vector<vk::QueueFamilyProperties> m_queueFamilies;
    std::vector<vk::ExtensionProperties> m_extensions;

    std::unordered_set<std::string_view> m_availableExtensionsSet;

    // Extensions features
    std::unordered_map<vk::StructureType, std::shared_ptr<void>> m_extensionFeatures;
    void* m_extensionFeaturePNext = nullptr;
    vk::PhysicalDeviceFeatures m_requiredFeatures;
};

// TODO: C++26 reflection
#define REQUEST_OPTIONAL_EXT_FEATURE(gpu, feature, flag) gpu->RequestOptionalExtensionFeature<feature>(&feature::flag, #feature, #flag)
#define REQUEST_REQUIRED_EXT_FEATURE(gpu, feature, flag) gpu->RequestRequiredExtensionFeature<feature>(&feature::flag, #feature, #flag)
#define REQUEST_OPTIONAL_FEATURE(gpu, flag) gpu->RequestOptionalFeature(&vk::PhysicalDeviceFeatures::flag, #flag)
#define REQUEST_REQUIRED_FEATURE(gpu, flag) gpu->RequestRequiredFeature(&vk::PhysicalDeviceFeatures::flag, #flag)
}
