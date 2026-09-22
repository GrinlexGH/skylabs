#pragma once
#include <SDL3/SDL_video.h>

#include "skylabs/base/window.hpp"

namespace sk::sdl {
class SK_BASE_PUBLIC_CLASS Window final : public IWindow {
public:
    Window() = delete;
    explicit Window(std::nullptr_t) noexcept { }
    explicit Window(const char* title, int w, int h, SDL_WindowFlags flags = 0);
    Window(const Window&) = delete;
    Window(Window&& other) noexcept;
    Window& operator=(const Window&) = delete;
    Window& operator=(Window&& rhs) noexcept;
    ~Window() override;

    [[nodiscard]] SDL_Window* operator*() const noexcept { return m_handle; }

    [[nodiscard]] utils::Extent2D DrawableSize() const override;
    [[nodiscard]] bool IsMinimized() const override;

private:
    SDL_Window* m_handle = nullptr;
};
}
