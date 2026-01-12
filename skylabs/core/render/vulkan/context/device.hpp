#pragma once
#include <skylabs/core/render/vulkan/context/physical_device.hpp>
#include <skylabs/core/render/vulkan/context/extensions.hpp>
#include <skylabs/core/render/vulkan/window.hpp>

namespace Vulkan {
class CQueue
{
public:
    explicit CQueue(std::nullptr_t) {}
    explicit CQueue(vk::raii::Queue queue, const std::uint32_t& familyIndex) : m_handle(std::move(queue)), m_familyIndex(familyIndex) {}
    CQueue(CQueue&) = delete;
    CQueue(CQueue&&) = default;
    CQueue& operator=(CQueue&) = delete;
    CQueue& operator=(CQueue&&) = default;
    ~CQueue() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::Queue& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::raii::Queue* { return &m_handle; }

    [[nodiscard]] auto FamilyIndex() const noexcept -> std::uint32_t { return m_familyIndex; }

private:
    vk::raii::Queue m_handle { nullptr };
    std::uint32_t m_familyIndex = 0;
};

class CDevice
{
public:
    using DeviceFeatures = vk::StructureChain<
        vk::PhysicalDeviceFeatures2KHR,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan14Features,
        vk::PhysicalDeviceDynamicRenderingFeaturesKHR
    >;

    struct CRequestedFeature
    {
        struct CFeatureConfig
        {
            const std::uint32_t m_apiVersion;
            const CPhysicalDevice& m_gpu;
            DeviceFeatures& m_features;
            std::vector<const char*>& m_deviceExtensions;
        };
        using enableFunc_t = bool (*)(const CFeatureConfig&);

        enableFunc_t m_enable;
        ::Utils::Requirement m_requirement;
    };

    explicit CDevice(std::nullptr_t) {}
    explicit CDevice(
        const IWindow* window,
        vk::Instance instance,
        const CPhysicalDevice& gpu,
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

    [[nodiscard]] auto GraphicsQueue() const noexcept -> const CQueue& { return m_graphicsQueue; }
    [[nodiscard]] auto PresentQueue() const noexcept -> const CQueue& { return m_presentQueue; }
    [[nodiscard]] auto ComputeQueue() const noexcept -> const CQueue& { return m_computeQueue; }

    template <typename Feature>
    [[nodiscard]] auto IsFeatureEnabled(vk::Bool32 Feature::* flag) const -> Feature {
        return m_enabledFeatures.get<Feature>().*flag;
    }

private:
    struct CQueueFamilyIndices
    {
        std::uint32_t m_graphicsFamily = 0;
        std::uint32_t m_presentFamily = 0;
        std::uint32_t m_computeFamily = 0;
    };

    [[nodiscard]] static auto GetQueueFamilies(const IWindow* window, vk::Instance instance, const CPhysicalDevice& gpu) -> CQueueFamilyIndices;

    vk::raii::Device m_handle { nullptr };

    std::uint32_t m_apiVersion = 0;
    std::vector<std::string> m_activeExtensions;
    DeviceFeatures m_enabledFeatures;

    CQueue m_graphicsQueue { nullptr };
    CQueue m_presentQueue { nullptr };
    CQueue m_computeQueue { nullptr };
};
}
