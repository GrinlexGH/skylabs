#include "SDL_window.hpp"

#include "SDL_vulkan.hpp"
#include "SDL_video.hpp"

#include <stdexcept>
#include <format>

namespace SDL
{

CVulkanWindow::CVulkanWindow(const char* title, int w, int h, SDL_WindowFlags flags) {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        throw std::runtime_error("Failed to create SDL window. SDL_Video is not initialized!");
    }

    flags |= SDL_WINDOW_VULKAN;
    m_window = SDL_CreateWindow(title, w, h, flags);
    if (!m_window) {
        throw std::runtime_error(std::format("Failed to create SDL window: {}!", SDL_GetError()));
    }
}

CVulkanWindow::~CVulkanWindow() {
    if (m_window) {
        SDL_DestroyWindow(m_window);
    }
}

std::vector<const char*> CVulkanWindow::GetRequiredInstanceExtensions() {
    return Vulkan::GetInstanceExtensions();
}

bool CVulkanWindow::GetPresentationSupport(const vk::Instance instance, const vk::PhysicalDevice physicalDevice, const uint32_t queueFamilyIndex) {
    return Vulkan::GetPresentationSupport(instance, physicalDevice, queueFamilyIndex);
}

void CVulkanWindow::CreateSurface(const vk::Instance instance) {
    m_surface = Vulkan::CreateSurface(m_window, instance);
}

void CVulkanWindow::DestroySurface(const vk::Instance instance) {
    Vulkan::DestroySurface(instance, m_surface);
}

void CVulkanWindow::GetDrawableSize(int* w, int* h) {
    GetWindowSizeInPixels(m_window, w, h);
}

}
