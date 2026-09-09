#pragma once
#include <skylabs/core/render/vulkan/context/surface.hpp>

namespace Vulkan {
class CPhysicalDevice {
public:
    explicit CPhysicalDevice(std::nullptr_t) { }
    explicit CPhysicalDevice(const vk::raii::Instance& instance,
                             const vk::PhysicalDevice& physicalDevice, std::string&& name,
                             const std::uint32_t apiVersion)
        : m_handle(instance, physicalDevice), m_name(std::move(name)), m_apiVersion(apiVersion) { }
    CPhysicalDevice(CPhysicalDevice&) = delete;
    CPhysicalDevice(CPhysicalDevice&&) = default;
    CPhysicalDevice& operator=(CPhysicalDevice&) = delete;
    CPhysicalDevice& operator=(CPhysicalDevice&&) = default;
    ~CPhysicalDevice() = default;

    [[nodiscard]] const vk::raii::PhysicalDevice& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::PhysicalDevice* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] const std::string& Name() const noexcept { return m_name; }
    [[nodiscard]] std::uint32_t ApiVersion() const noexcept { return m_apiVersion; }

private:
    vk::raii::PhysicalDevice m_handle { nullptr };
    std::string m_name;
    std::uint32_t m_apiVersion = 0;
};
}
