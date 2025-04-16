#pragma once
#include <map>

#include "../renderer.hpp"
#include "instance.hpp"
#include "console.hpp"
#include "extensions/extensions.hpp"

namespace Vulkan {
class CPhysicalDevice
{
public:
    explicit CPhysicalDevice(const vk::PhysicalDevice& physicalDevice);
    CPhysicalDevice(const CPhysicalDevice&) = delete;
    CPhysicalDevice(CPhysicalDevice&&) = delete;
    CPhysicalDevice& operator=(const CPhysicalDevice&) = delete;
    CPhysicalDevice& operator=(CPhysicalDevice&&) = delete;
    ~CPhysicalDevice() = default;

    [[nodiscard]] vk::PhysicalDevice GetHandle() const { return m_handle; }

    [[nodiscard]] const vk::PhysicalDeviceProperties& GetProperties() const { return m_properties; }
    [[nodiscard]] const vk::PhysicalDeviceFeatures& GetFeatures() const { return m_features; }
    [[nodiscard]] const std::vector<vk::ExtensionProperties>& GetExtensions() const { return m_extensions; }
    [[nodiscard]] const std::vector<vk::QueueFamilyProperties>& GetQueueFamilies() const { return m_queueFamilies; }

    [[nodiscard]] bool IsExtensionSupported(const char* name) const { return HasExtension(m_extensions, name); }

    [[nodiscard]] const vk::PhysicalDeviceFeatures& GetRequiredFeatures() const { return m_requiredFeatures; }
    [[nodiscard]] void* GetExtensionFeaturePNext() const { return m_extensionFeaturePNext; }

    template <typename FeatureStructureType>
    FeatureStructureType GetExtensionFeatures() {
        // VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME required
        return m_handle.getFeatures2KHR<vk::PhysicalDeviceFeatures2KHR, FeatureStructureType>().template get<FeatureStructureType>();
    }

    template <typename FeatureStructureType>
    FeatureStructureType& AddExtensionFeatures() {
        auto [it, added] = m_extensionFeatures.try_emplace({ FeatureStructureType::structureType, std::make_shared<FeatureStructureType>() });
        if (added) {
            AppendToPNextChain(m_extensionFeaturePNext, it->second.get());
        }

        return *static_cast<FeatureStructureType*>(it->second.get());
    }

    template <typename Feature>
    void RequestRequiredExtensionFeature(vk::Bool32 Feature::* flag, const std::string& featureName, const std::string& flagName) {
        if (GetExtensionFeatures<Feature>().*flag) {
            AddExtensionFeatures<Feature>().*flag = true;
        } else {
            throw CRendererInitError(
                std::format(
                    "Requested required feature <{}::{}> is not supported!",
                    featureName, flagName
                )
            );
        }
    }

    template <typename Feature>
    vk::Bool32 RequestOptionalExtensionFeature(vk::Bool32 Feature::* flag, const std::string& featureName, const std::string& flagName) {
        const vk::Bool32 supported = GetExtensionFeatures<Feature>().*flag;
        if (supported) {
            AddExtensionFeatures<Feature>().*flag = true;
        } else {
            Log::Info("Requested optional feature <{}::{}> is not supported", featureName, flagName);
        }

        return supported;
    }

    void RequestRequiredFeature(vk::Bool32 vk::PhysicalDeviceFeatures::* flag, const std::string& flagName) {
        if (m_features.*flag) {
            m_requiredFeatures.*flag = true;
        } else {
            throw CRendererInitError(
                std::format(
                    "Requested required feature \"{}\" is not supported!",
                    flagName
                )
            );
        }
    }

    vk::Bool32 RequestOptionalFeature(vk::Bool32 vk::PhysicalDeviceFeatures::* flag, const std::string& flagName) {
        const vk::Bool32 supported = m_features.*flag;
        if (supported) {
            m_requiredFeatures.*flag = true;
        } else {
            Log::Info("Requested optional feature \"{}\" is not supported", flagName);
        }

        return supported;
    }

private:
    vk::PhysicalDevice m_handle = VK_NULL_HANDLE;

    // properties
    const vk::PhysicalDeviceProperties m_properties {};
    const vk::PhysicalDeviceFeatures m_features {};
    const std::vector<vk::QueueFamilyProperties> m_queueFamilies {};
    const std::vector<vk::ExtensionProperties> m_extensions {};

    std::map<vk::StructureType, std::shared_ptr<void>> m_extensionFeatures {};
    void* m_extensionFeaturePNext = nullptr;
    vk::PhysicalDeviceFeatures m_requiredFeatures {};
};

#define REQUEST_OPTIONAL_EXT_FEATURE(gpu, Feature, flag) gpu->RequestOptionalExtensionFeature<Feature>(&Feature::flag, #Feature, #flag)
#define REQUEST_REQUIRED_EXT_FEATURE(gpu, Feature, flag) gpu->RequestRequiredExtensionFeature<Feature>(&Feature::flag, #Feature, #flag)
#define REQUEST_OPTIONAL_FEATURE(gpu, flag) gpu->RequestOptionalFeature(&vk::PhysicalDeviceFeatures::flag, #flag)
#define REQUEST_REQUIRED_FEATURE(gpu, flag) gpu->RequestRequiredFeature(&vk::PhysicalDeviceFeatures::flag, #flag)
}
