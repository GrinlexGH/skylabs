#include "SDL_window.hpp"

#include "SDL_vulkan.hpp"
#include "SDL_video.hpp"

#include <stdexcept>
#include <format>

#include <console.hpp>

namespace SDL {
CVulkanWindow::CVulkanWindow(const char* title, const int w, const int h, SDL_WindowFlags flags) {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        throw std::runtime_error("Failed to create SDL window. SDL_Video was not initialized!");
    }

    flags |= SDL_WINDOW_VULKAN;
    m_ptr = SDL_CreateWindow(title, w, h, flags);
    if (!m_ptr) {
        throw std::runtime_error(std::format("Failed to create SDL window: {}!", SDL_GetError()));
    }
}

CVulkanWindow::~CVulkanWindow() {
    if (m_surface) {
        Warning("Window was deleted before surface was deleted.");
    }

    if (m_ptr) {
        SDL_DestroyWindow(m_ptr);
    }
}

std::vector<const char*> CVulkanWindow::GetRequiredInstanceExtensions() const {
    return Vulkan::GetInstanceExtensions();
}

bool CVulkanWindow::CheckQueuePresentSupport(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, const uint32_t queueFamilyIndex) const {
    return Vulkan::GetPresentationSupport(instance, physicalDevice, queueFamilyIndex);
}

void CVulkanWindow::CreateSurface(const vk::Instance& instance) {
    m_surface = Vulkan::CreateSurface(m_ptr, instance);
}

void CVulkanWindow::DestroySurface(const vk::Instance& instance) {
    Vulkan::DestroySurface(instance, m_surface);
}

void CVulkanWindow::GetDrawableSize(int* w, int* h) {
    GetWindowSizeInPixels(m_ptr, w, h);
}
}
