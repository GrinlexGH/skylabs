#pragma once

#include <SDL3/SDL.h>

#include "window.hpp"

namespace SDL {
    class CHandle {
    private:
        static int m_instanceCount;

    public:
        CHandle();
        CHandle(const CHandle&) noexcept { ++m_instanceCount; }
        CHandle(CHandle&&) noexcept { ++m_instanceCount; }
        CHandle& operator=(const CHandle&) = default;
        CHandle& operator=(CHandle&&) = default;
        virtual ~CHandle();

        static int InstanceCount() { return m_instanceCount; }
    };

    class CWindow final : public IWindow {
    private:
        SDL_Window* m_window = nullptr;

    public:
        using IWindow::IWindow;

        CWindow() = default;
        CWindow(const char* title, int x, int y, int w, int h, unsigned int SDLflags);
        CWindow(const CWindow&) = delete;
        CWindow(CWindow&& window) noexcept;
        CWindow& operator=(const CWindow&) = delete;
        CWindow& operator=(CWindow&& window) noexcept;
        ~CWindow();

        void Create(const char* title, int x, int y, int w, int h, unsigned int SDLflags) override;
        void Close() override;
        void* GetHandle() const override { return m_window; }
        WindowVendor GetVendor() const override { return WindowVendor::eSDL; }
    };
}
