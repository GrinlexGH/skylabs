#include <skylabs/public/sdl/window.hpp>
#include <skylabs/public/sdl/sdl.hpp>

namespace SDL {
CWindow::CWindow(const char* title, const int w, const int h, const SDL_WindowFlags flags) :
    m_handle(SDL_CreateWindow(title, w, h, flags))
{
    if (!m_handle) {
        throw std::runtime_error(fmt::format("Failed to create SDL window: {}", SDL_GetError()));
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

CWindow::~CWindow() {
    if(m_handle) { SDL_DestroyWindow(m_handle); }
}

Utils::Extent2D CWindow::DrawableSize() const {
    return GetWindowSizeInPixels(m_handle);
}

// Vulkan
vk::SurfaceKHR CWindow::CreateSurface(const vk::Instance instance) const {
    return Vulkan::CreateSurface(m_handle, instance);
}
}
