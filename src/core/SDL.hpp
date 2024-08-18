#pragma once

#include <SDL.h>

#include "window.hpp"

namespace SDL {
    class Handle {
    private:
        static int instanceCount_;

    public:
        Handle();
        Handle(const Handle&) noexcept { ++instanceCount_; }
        Handle(Handle&&) noexcept { ++instanceCount_; }
        Handle& operator=(const Handle&) = default;
        Handle& operator=(Handle&&) = default;
        virtual ~Handle();

        static int InstanceCount() { return instanceCount_; }
    };

    class CWindow final : public IWindow {
    private:
        SDL_Window* window_ = nullptr;

    public:
        CWindow() = default;
        CWindow(const char* title, int x, int y, int w, int h, unsigned int SDLflags);
        CWindow(const CWindow&) = delete;
        CWindow(CWindow&& window) noexcept;
        CWindow& operator=(const CWindow&) = delete;
        CWindow& operator=(CWindow&& window) noexcept;
        ~CWindow();

        void Create(const char* title, int x, int y, int w, int h, unsigned int SDLflags) override;
        void Close() override;
        void* GetHandle() const override { return window_; }
    };
}
