#pragma once

#include "../vulkan_window.hpp"

#include <SDL3/SDL.h>

namespace SDL
{

class CVulkanWindow final : public IVulkanWindow {
public:
    CVulkanWindow() = default;
    CVulkanWindow(const char* title, int w, int h, SDL_WindowFlags flags = 0);
    CVulkanWindow(const CVulkanWindow&) = default;
    CVulkanWindow(CVulkanWindow&&) = default;
    CVulkanWindow& operator=(const CVulkanWindow&) = default;
    CVulkanWindow& operator=(CVulkanWindow&&) = default;
    ~CVulkanWindow() override;

    std::vector<const char*> GetRequiredInstanceExtensions() override;
    bool GetPresentationSupport(vk::Instance instance, vk::PhysicalDevice physicalDevice, uint32_t queueFamilyIndex) override;
    void CreateSurface(vk::Instance instance) override;
    void DestroySurface(vk::Instance instance) override;
    void GetDrawableSize(int* w, int* h) override;

    SDL_Window* m_window = nullptr;
};

}
