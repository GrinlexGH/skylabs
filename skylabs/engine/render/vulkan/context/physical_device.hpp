#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace sk::render::vulkan {
class PhysicalDevice {
public:
    explicit PhysicalDevice(std::nullptr_t) { }
    explicit PhysicalDevice(const vk::raii::Instance& instance, const vk::PhysicalDevice& physicalDevice,
                            std::string&& name)
        : m_handle(instance, physicalDevice), m_name(std::move(name)) { }
    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice(PhysicalDevice&&) noexcept = default;
    PhysicalDevice& operator=(const PhysicalDevice&) = delete;
    PhysicalDevice& operator=(PhysicalDevice&&) noexcept = default;
    ~PhysicalDevice() = default;

    [[nodiscard]] const vk::raii::PhysicalDevice& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::PhysicalDevice* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] const std::string& Name() const noexcept { return m_name; }

private:
    vk::raii::PhysicalDevice m_handle { nullptr };
    std::string m_name;
};
}
