#pragma once
#include "instance.hpp"
#include "logging.hpp"
#include "extensions.hpp"

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

    [[nodiscard]] const vk::raii::PhysicalDevice& GetHandle() const { return m_handle; }

    [[nodiscard]] const vk::PhysicalDeviceProperties& GetProperties() const { return m_properties; }
    [[nodiscard]] const vk::PhysicalDeviceFeatures& GetFeatures() const { return m_features; }
    [[nodiscard]] const std::vector<vk::ExtensionProperties>& GetExtensions() const { return m_extensions; }
    [[nodiscard]] const std::vector<vk::QueueFamilyProperties>& GetQueueFamilies() const { return m_queueFamilies; }

    [[nodiscard]] bool IsExtensionSupported(const char* name) const { return HasExtension(m_extensions, name); }

    [[nodiscard]] const vk::PhysicalDeviceFeatures& GetRequiredFeatures() const { return m_requiredFeatures; }
    [[nodiscard]] void* GetExtensionFeaturePNext() const { return m_extensionFeaturePNext; }

    template <typename Feature>
    [[nodiscard]] Feature GetExtensionFeatures() const {
        // VK_KHR_get_physical_device_properties2 is required
        return m_handle.getFeatures2KHR<vk::PhysicalDeviceFeatures2KHR, Feature>().template get<Feature>();
    }

    template <typename Feature>
    [[nodiscard]] vk::Bool32 IsExtensionFeatureEnabled(vk::Bool32 Feature::* flag) const {
        auto it = m_extensionFeatures.find(Feature::structureType);
        if (it != m_extensionFeatures.end() && it->second) {
            auto feature = std::static_pointer_cast<Feature>(it->second);
            return (*feature).*flag;
        }
        return vk::False;
    }

    template <typename Feature>
    Feature& AddExtensionFeatures() {
        auto [it, added] = m_extensionFeatures.try_emplace(Feature::structureType, std::make_shared<Feature>());
        if (added) {
            AppendToPNextChain(m_extensionFeaturePNext, it->second.get());
        }

        return *static_cast<Feature*>(it->second.get());
    }

    template <typename Feature>
    void RequestRequiredExtensionFeature(vk::Bool32 Feature::* flag, const char* featureName, const char* flagName) {
        if (GetExtensionFeatures<Feature>().*flag) {
            AddExtensionFeatures<Feature>().*flag = vk::True;
        } else {
            throw std::runtime_error(
                std::format("Requested required feature <{}::{}> is not supported!", featureName, flagName)
            );
        }
    }

    template <typename Feature>
    vk::Bool32 RequestOptionalExtensionFeature(vk::Bool32 Feature::* flag, const char* featureName, const char* flagName) {
        const vk::Bool32 supported = GetExtensionFeatures<Feature>().*flag;
        if (supported) {
            AddExtensionFeatures<Feature>().*flag = vk::True;
        } else {
            Log::Info("Requested optional feature <{}::{}> is not supported", featureName, flagName);
        }

        return supported;
    }

    void RequestRequiredFeature(vk::Bool32 vk::PhysicalDeviceFeatures::* flag, const char* flagName) {
        if (m_features.*flag) {
            m_requiredFeatures.*flag = vk::True;
        } else {
            throw std::runtime_error(
                std::format("Requested required feature \"{}\" is not supported!", flagName)
            );
        }
    }

    vk::Bool32 RequestOptionalFeature(vk::Bool32 vk::PhysicalDeviceFeatures::* flag, const char* flagName) {
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

    // Extensions features
    std::unordered_map<vk::StructureType, std::shared_ptr<void>> m_extensionFeatures;
    void* m_extensionFeaturePNext = nullptr;
    vk::PhysicalDeviceFeatures m_requiredFeatures;
};

#define REQUEST_OPTIONAL_EXT_FEATURE(gpu, feature, flag) gpu->RequestOptionalExtensionFeature<feature>(&feature::flag, #feature, #flag)
#define REQUEST_REQUIRED_EXT_FEATURE(gpu, feature, flag) gpu->RequestRequiredExtensionFeature<feature>(&feature::flag, #feature, #flag)
#define REQUEST_OPTIONAL_FEATURE(gpu, flag) gpu->RequestOptionalFeature(&vk::PhysicalDeviceFeatures::flag, #flag)
#define REQUEST_REQUIRED_FEATURE(gpu, flag) gpu->RequestRequiredFeature(&vk::PhysicalDeviceFeatures::flag, #flag)
}
