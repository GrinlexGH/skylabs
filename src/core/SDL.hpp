#pragma once

#include <SDL.h>

#include "window.hpp"

namespace SDL {
    class Handle {
    private:
        static unsigned int instanceCount_;
    public:
        Handle();
        Handle(const Handle&)               { ++instanceCount_; }
        Handle(Handle&&)                    { ++instanceCount_; }
        Handle& operator=(const Handle&)    = default;
        Handle& operator=(Handle&&)         = default;
        virtual ~Handle();

        static unsigned int InstanceCount() { return instanceCount_; }
    };

    class CWindow final : public IWindow {
    private:
        SDL_Window* window_ = nullptr;
    public:
        CWindow() = default;
        CWindow(const char* title, int x, int y, int w, int h, unsigned int SDLflags);
        ~CWindow();
        void Create(const char* title, int x, int y, int w, int h, unsigned int SDLflags) override;
        void Close() override;
        void* GetHandle() const override { return window_; }
    };
}
