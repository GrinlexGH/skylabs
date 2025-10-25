#include <skylabs/core/SDL/vulkan/window.hpp>

#include <skylabs/core/SDL/video.hpp>
#include <skylabs/core/SDL/vulkan/vulkan.hpp>

#include <stdexcept>
#include <format>

namespace SDL::Vulkan {
CWindow::CWindow(const char* title, const int w, const int h, const SDL_WindowFlags flags) :
    m_handle(SDL_CreateWindow(title, w, h, SDL_WINDOW_VULKAN | flags))
{
    if (!m_handle) {
        throw std::runtime_error(std::format("Failed to create SDL window: {}!", SDL_GetError()));
    }
}

CWindow::CWindow(CWindow&& other) noexcept :
    m_handle(std::exchange(other.m_handle, nullptr))
{}

CWindow& CWindow::operator=(CWindow&& rhs) noexcept {
    if (this != &rhs) {
        if (m_handle) { SDL_DestroyWindow(m_handle); }
        m_handle = std::exchange(rhs.m_handle, nullptr);
    }
    return *this;
}

auto CWindow::GetRequiredInstanceExtensions() const -> std::span<const char* const> {
    return GetInstanceExtensions();
}

auto CWindow::IsQueueFamilyPresentSupport(
    const vk::Instance& instance,
    const vk::PhysicalDevice& physicalDevice,
    const uint32_t index
) const -> bool {
    return GetPresentationSupport(instance, physicalDevice, index);
}

auto CWindow::CreateSurface(const vk::Instance& instance) const -> vk::SurfaceKHR {
    return Vulkan::CreateSurface(m_handle, instance);
}

auto CWindow::DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const -> void {
    Vulkan::DestroySurface(instance, surface);
    surface = nullptr;
}

auto CWindow::GetDrawableSize(int& w, int& h) const -> void {
    GetWindowSizeInPixels(m_handle, &w, &h);
}

CWindow::~CWindow() {
    if(m_handle) {
        SDL_DestroyWindow(m_handle);
    }
}
}
