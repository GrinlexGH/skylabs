#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace sk::render::vulkan {
class PhysicalDevice {
public:
    explicit PhysicalDevice(std::nullptr_t) { }
    explicit PhysicalDevice(const vk::raii::Instance& instance, const vk::PhysicalDevice& physicalDevice,
                            std::string&& name, const std::uint32_t apiVersion)
        : m_handle(instance, physicalDevice), m_name(std::move(name)), m_apiVersion(apiVersion) { }
    PhysicalDevice(PhysicalDevice&) = delete;
    PhysicalDevice(PhysicalDevice&&) = default;
    PhysicalDevice& operator=(PhysicalDevice&) = delete;
    PhysicalDevice& operator=(PhysicalDevice&&) = default;
    ~PhysicalDevice() = default;

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
