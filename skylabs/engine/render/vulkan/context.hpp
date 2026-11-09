#pragma once
#include "skylabs/engine/render/vulkan/os_adapter.hpp"
#include "skylabs/engine/window.hpp"

namespace sk::render::vulkan {
class Queue {
public:
    explicit Queue(const vk::raii::Device& device, const vk::Queue& queue,
                   const std::uint32_t familyIndex)
        : m_handle(device, queue), m_familyIndex(familyIndex) { }

    [[nodiscard]] const vk::raii::Queue& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Queue* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] std::uint32_t FamilyIndex() const noexcept { return m_familyIndex; }

private:
    vk::raii::Queue m_handle;
    std::uint32_t m_familyIndex;
};

struct Queues {
    Queue compute;
    Queue graphics;
    Queue present;
};

struct DeviceCaps {
    std::uint32_t apiVersion = vk::ApiVersion10;
    bool debugUtils = false;
    bool maintenance5 = false;
    bool extendedFlags = false;
    bool memoryPriority = false;
    bool descriptorHeap = false;
};

class Device {
public:
    explicit Device(vk::raii::Device&& device, const DeviceCaps& caps)
        : m_handle(std::move(device)), m_caps(caps) { }

    [[nodiscard]] const vk::raii::Device& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Device* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] DeviceCaps Caps() const noexcept { return m_caps; }

private:
    vk::raii::Device m_handle;
    DeviceCaps m_caps;
};

class Context {
public:
    explicit Context(const IWindow* window, const IOSAdapter* osAdapter);

    [[nodiscard]] const IWindow* Window() const noexcept { return m_window; }
    [[nodiscard]] const vk::raii::Instance& Instance() const noexcept { return m_instance; }
    [[nodiscard]] const vk::raii::SurfaceKHR& Surface() const noexcept { return m_surface; }
    [[nodiscard]] const vk::raii::PhysicalDevice& PhysicalDevice() const noexcept {
        return m_physicalDevice;
    }

    [[nodiscard]] const Device& GetDevice() const noexcept { return m_device; }
    [[nodiscard]] const Queues& GetQueues() const noexcept { return m_queues; }
    [[nodiscard]] const vma::raii::Allocator& Allocator() const noexcept { return m_allocator; }

    void RecreateSurface();

private:
    struct InitData;
    static InitData Initialize(const IWindow* window, const IOSAdapter* osAdapter);
    Context(InitData&& initData);

    const IWindow* m_window;
    const IOSAdapter* m_osAdapter;
    vk::raii::Instance m_instance;
    vk::raii::DebugUtilsMessengerEXT m_debugUtilsMessenger;
    vk::raii::SurfaceKHR m_surface;
    vk::raii::PhysicalDevice m_physicalDevice;
    Device m_device;
    Queues m_queues;
    vma::raii::Allocator m_allocator;
};
}
