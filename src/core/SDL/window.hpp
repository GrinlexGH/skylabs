#pragma once
#include "../render/vulkan/vulkan_window.hpp"

#include <SDL3/SDL.h>

namespace SDL {
class CVulkanWindow final : public IVulkanWindow
{
public:
    explicit CVulkanWindow(const char* title, int w, int h, SDL_WindowFlags flags = 0);
    CVulkanWindow(const CVulkanWindow&) = delete;
    CVulkanWindow(CVulkanWindow&& other) noexcept;
    CVulkanWindow& operator=(const CVulkanWindow&) = delete;
    CVulkanWindow& operator=(CVulkanWindow&& rhs) noexcept;
    ~CVulkanWindow() override;

    [[nodiscard]] SDL_Window* GetHandle() const { return m_handle; }

    [[nodiscard]] std::vector<const char*> GetRequiredInstanceExtensions() const override;
    [[nodiscard]] bool CheckQueuePresentSupport(
        const vk::Instance& instance,
        const vk::PhysicalDevice& physicalDevice,
        std::uint32_t queueFamilyIndex
    ) const override;

    [[nodiscard]] vk::SurfaceKHR CreateSurface(const vk::Instance& instance) const override;
    void DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const override;

    void GetDrawableSize(int* w, int* h) const override;

private:
    SDL_Window* m_handle = nullptr;
};
}
