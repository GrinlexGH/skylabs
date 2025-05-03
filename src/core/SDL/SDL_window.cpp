#include "SDL_window.hpp"

#include "SDL_vulkan.hpp"
#include "SDL_video.hpp"

#include <stdexcept>
#include <format>

#include "logging.hpp"

namespace SDL {
CVulkanWindow::CVulkanWindow(const char* title, const int w, const int h, SDL_WindowFlags flags) {
    flags |= SDL_WINDOW_VULKAN;
    m_ptr = SDL_CreateWindow(title, w, h, flags);
    if (!m_ptr) {
        throw std::runtime_error(std::format("Failed to create SDL window: {}!", SDL_GetError()));
    }
}

CVulkanWindow::~CVulkanWindow() {
    if (m_ptr) {
        SDL_DestroyWindow(m_ptr);
    }
}

std::vector<const char*> CVulkanWindow::GetRequiredInstanceExtensions() const {
    return Vulkan::GetInstanceExtensions();
}

bool CVulkanWindow::CheckQueuePresentSupport(
    const vk::Instance& instance,
    const vk::PhysicalDevice& physicalDevice,
    const uint32_t queueFamilyIndex
) const {
    return Vulkan::GetPresentationSupport(instance, physicalDevice, queueFamilyIndex);
}

vk::SurfaceKHR CVulkanWindow::CreateSurface(const vk::Instance& instance) const {
    return Vulkan::CreateSurface(m_ptr, instance);
}

void CVulkanWindow::DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const {
    Vulkan::DestroySurface(instance, surface);
    surface = VK_NULL_HANDLE;
}

void CVulkanWindow::GetDrawableSize(int* w, int* h) {
    GetWindowSizeInPixels(m_ptr, w, h);
}
}
