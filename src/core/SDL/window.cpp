#include "window.hpp"

#include <stdexcept>
#include <format>

#include "video.hpp"

namespace SDL {
CVulkanWindow::CVulkanWindow(const char* title, const int w, const int h, SDL_WindowFlags flags) :
    m_handle(SDL_CreateWindow(title, w, h, flags))
{
    if (!m_handle) {
        throw std::runtime_error(std::format("Failed to create SDL window: {}!", SDL_GetError()));
    }
}

CVulkanWindow::CVulkanWindow(CVulkanWindow&& other) noexcept :
    m_handle(std::exchange(other.m_handle, nullptr))
{ }

CVulkanWindow& CVulkanWindow::operator=(CVulkanWindow&& rhs) noexcept {
    if (this != &rhs) {
        if (m_handle) { SDL_DestroyWindow(m_handle); }
        m_handle = std::exchange(rhs.m_handle, nullptr);
    }
    return *this;
}
//
// std::vector<const char*> CVulkanWindow::GetRequiredInstanceExtensions() const {
//     return Vulkan::GetInstanceExtensions();
// }
//
// bool CVulkanWindow::CheckQueuePresentSupport(
//     const vk::Instance& instance,
//     const vk::PhysicalDevice& physicalDevice,
//     const uint32_t queueFamilyIndex
// ) const {
//     return Vulkan::GetPresentationSupport(instance, physicalDevice, queueFamilyIndex);
// }
//
// vk::SurfaceKHR CVulkanWindow::CreateSurface(const vk::Instance& instance) const {
//     return Vulkan::CreateSurface(m_handle, instance);
// }
//
// void CVulkanWindow::DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const {
//     Vulkan::DestroySurface(instance, surface);
//     surface = VK_NULL_HANDLE;
// }
//
void CVulkanWindow::GetDrawableSize(int* w, int* h) const {
    GetWindowSizeInPixels(m_handle, w, h);
}

CVulkanWindow::~CVulkanWindow() {
    if(m_handle) {
        SDL_DestroyWindow(m_handle);
    }
}
}
