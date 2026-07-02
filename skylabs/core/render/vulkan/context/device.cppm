module;
#include <skylabs/core/pch.hpp>
export module skylabs.vulkan.context:device;
export import :physical_device;

export namespace Vulkan {
class CQueue
{
public:
    explicit CQueue(std::nullptr_t) {}
    explicit CQueue(
        const vk::raii::Device& device,
        const VkQueue queue,
        const std::uint32_t familyIndex
    ) : m_handle(device, queue), m_familyIndex(familyIndex) {}
    CQueue(CQueue&) = delete;
    CQueue(CQueue&&) = default;
    CQueue& operator=(CQueue&) = delete;
    CQueue& operator=(CQueue&&) = default;
    ~CQueue() = default;

    [[nodiscard]] const vk::raii::Queue& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Queue* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] std::uint32_t FamilyIndex() const noexcept { return m_familyIndex; }

private:
    vk::raii::Queue m_handle { nullptr };
    std::uint32_t m_familyIndex = 0;
};

struct DeviceCaps
{
    bool m_maintenance5 = false;
    bool m_samplerAnisotropy = false;
};

class CDevice
{
public:
    explicit CDevice(std::nullptr_t) {}
    explicit CDevice(const CInstance& instance, CPhysicalDevice& physicalDevice);
    CDevice(CDevice&) = delete;
    CDevice(CDevice&&) = default;
    CDevice& operator=(CDevice&) = delete;
    CDevice& operator=(CDevice&&) = default;
    ~CDevice() = default;

    [[nodiscard]] const vk::raii::Device& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Device* operator->() const noexcept { return &m_handle; }
    [[nodiscard]] const vkb::Device& VkbDevice() const noexcept { return m_vkbDevice; }
    [[nodiscard]] vkb::Device& VkbDevice() noexcept { return m_vkbDevice; }

    [[nodiscard]] const CQueue& GraphicsQueue() const noexcept { return m_graphicsQueue; }
    [[nodiscard]] const CQueue& PresentQueue() const noexcept { return m_presentQueue; }
    [[nodiscard]] const CQueue& ComputeQueue() const noexcept { return m_computeQueue; }

    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const { return m_enabledExtensions.contains(name); }
    [[nodiscard]] DeviceCaps Caps() const noexcept { return m_caps; }

private:
    vk::raii::Device m_handle { nullptr };
    vkb::Device m_vkbDevice;

    boost::container::flat_set<std::string, std::less<>> m_enabledExtensions;
    DeviceCaps m_caps;

    CQueue m_graphicsQueue { nullptr };
    CQueue m_presentQueue { nullptr };
    CQueue m_computeQueue { nullptr };
};
}
