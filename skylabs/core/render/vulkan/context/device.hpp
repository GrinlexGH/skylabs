#pragma once
#include <skylabs/core/render/vulkan/context/physical_device.hpp>
#include <skylabs/core/render/vulkan/context/extensions.hpp>
#include <skylabs/core/render/vulkan/window.hpp>

namespace Vulkan {
class CDevice
{
public:
    using DeviceFeatures = vk::StructureChain<
        vk::PhysicalDeviceFeatures2KHR,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan14Features,
        vk::PhysicalDeviceDynamicRenderingFeaturesKHR>;

    struct CRequestedFeature
    {
        using PFN_enable = bool (*)(
            std::uint32_t apiVersion,
            const CPhysicalDevice& gpu,
            DeviceFeatures& features,
            std::vector<const char*>& deviceExtensions
        );

        PFN_enable m_enable;
        Utils::Requirement m_requirement;
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

    [[nodiscard]] auto GraphicsQueue() const noexcept -> const vk::raii::Queue& { return m_graphicsQueue; }
    [[nodiscard]] auto PresentQueue() const noexcept -> const vk::raii::Queue& { return m_presentQueue; }
    [[nodiscard]] auto ComputeQueue() const noexcept -> const vk::raii::Queue& { return m_computeQueue; }

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

    [[nodiscard]] static auto GetQueueCreateInfos(
        const IWindow* window,
        vk::Instance instance,
        const CPhysicalDevice& gpu
    ) -> std::pair<std::vector<vk::DeviceQueueCreateInfo>, CQueueFamilyIndices>;

    vk::raii::Device m_handle { nullptr };

    std::uint32_t m_apiVersion = 0;
    std::vector<std::string> m_activeExtensions;
    DeviceFeatures m_enabledFeatures;

    vk::raii::Queue m_graphicsQueue { nullptr };
    vk::raii::Queue m_presentQueue { nullptr };
    vk::raii::Queue m_computeQueue { nullptr };
};
}
