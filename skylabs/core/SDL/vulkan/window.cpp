#include <skylabs/core/SDL/vulkan/window.hpp>

#include <skylabs/core/SDL/video.hpp>
#include <skylabs/core/SDL/vulkan/vulkan.hpp>

#include <stdexcept>

#include <fmt/format.h>

namespace SDL::Vulkan {
CWindow::CWindow(const char* title, const int w, const int h, const SDL_WindowFlags flags) :
    m_handle(SDL_CreateWindow(title, w, h, SDL_WINDOW_VULKAN | flags))
{
    if (!m_handle) {
        throw std::runtime_error(fmt::format("Failed to create SDL window: {}!", SDL_GetError()));
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

std::span<const char* const> CWindow::GetRequiredInstanceExtensions() const {
    return GetInstanceExtensions();
}

bool CWindow::IsQueueFamilySupportPresent(
    const vk::Instance& instance,
    const vk::PhysicalDevice& physicalDevice,
    const uint32_t index
) const {
    return GetPresentationSupport(instance, physicalDevice, index);
}

vk::SurfaceKHR CWindow::CreateSurface(const vk::Instance& instance) const {
    return Vulkan::CreateSurface(m_handle, instance);
}

void CWindow::DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const {
    Vulkan::DestroySurface(instance, surface);
    surface = nullptr;
}

Utils::CExtent2D CWindow::DrawableSize() const {
    return GetWindowSizeInPixels(m_handle);
}

CWindow::~CWindow() {
    if(m_handle) {
        SDL_DestroyWindow(m_handle);
    }
}
}
