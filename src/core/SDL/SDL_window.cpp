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
    SDL_DestroyWindow(m_window);
}

std::vector<const char*> CVulkanWindow::GetRequiredInstanceExtensions() {
    return SDL::Vulkan::GetInstanceExtensions();
}

bool CVulkanWindow::GetPresentationSupport(vk::Instance instance, vk::PhysicalDevice physicalDevice, uint32_t queueFamilyIndex) {
    return SDL::Vulkan::GetPresentationSupport(instance, physicalDevice, queueFamilyIndex);
}

void CVulkanWindow::CreateSurface(vk::Instance instance) {
    m_surface = SDL::Vulkan::CreateSurface(m_window, instance);
}

void CVulkanWindow::DestroySurface(vk::Instance instance) {
    SDL::Vulkan::DestroySurface(instance, m_surface);
}

void CVulkanWindow::GetDrawableSize(int* w, int* h) {
    SDL::GetWindowSizeInPixels(m_window, w, h);
}

}
