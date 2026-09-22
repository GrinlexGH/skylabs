#include <stdexcept>
#include <utility>

#include <SDL3/SDL_error.h>
#include <fmt/format.h>

#include "skylabs/base/sdl/sdl.hpp"
#include "skylabs/base/sdl/window.hpp"

namespace sk::sdl {
Window::Window(const char* title, const int w, const int h, const SDL_WindowFlags flags) {
#ifdef PLATFORM_ANDROID
    flags |= SDL_WINDOW_FULLSCREEN;
#endif

    m_handle = SDL_CreateWindow(title, w, h, flags);

    if (!m_handle) {
        throw std::runtime_error(fmt::format("Failed to create SDL window: {}", SDL_GetError()));
    }
}

Window::Window(Window&& other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) { }

Window& Window::operator=(Window&& rhs) noexcept {
    if (this != &rhs) {
        if (m_handle) {
            SDL_DestroyWindow(m_handle);
        }
        m_handle = std::exchange(rhs.m_handle, nullptr);
    }
    return *this;
}

Window::~Window() {
    if (m_handle) {
        SDL_DestroyWindow(m_handle);
    }
}

utils::Extent2D Window::DrawableSize() const { return GetWindowSizeInPixels(m_handle); }
bool Window::IsMinimized() const { return SDL_GetWindowFlags(m_handle) & SDL_WINDOW_MINIMIZED; }
}
