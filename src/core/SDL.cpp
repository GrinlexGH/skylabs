#include "SDL.hpp"

#include <stdexcept>

//============
// SDL CHandle
int SDL::CHandle::m_instanceCount = 0;

SDL::CHandle::CHandle() {
    if (m_instanceCount == 0) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            throw std::runtime_error(
                std::string("Couldn't initialize SDL!\n") + SDL_GetError()
            );
        }
    }

    ++m_instanceCount;
}

SDL::CHandle::~CHandle() {
    --m_instanceCount;
    if (m_instanceCount == 0) {
        SDL_Quit();
    }
}

//============
// SDL Window
SDL::CWindow::CWindow(CWindow&& window) noexcept {
    m_window = window.m_window;
    window.m_window = nullptr;
};

SDL::CWindow& SDL::CWindow::operator=(CWindow&& window) noexcept {
    m_window = window.m_window;
    window.m_window = nullptr;
    return *this;
}

SDL::CWindow::CWindow(const char* title, int x, int y, int w, int h, unsigned int SDLflags) {
    Create(title, x, y, w, h, SDLflags);
}

void SDL::CWindow::Create(const char* title, int x, int y, int w, int h, unsigned int SDLflags) {
    if (m_window) {
        return;
    }

    if (SDL::CHandle::InstanceCount() == 0) {
        throw std::runtime_error(
            "Cant create window: you must create SDL::CHandle object!"
        );
    }

    m_window = SDL_CreateWindow(title, x, y, w, h, SDLflags);
}

SDL::CWindow::~CWindow() {
    Close();
}

void SDL::CWindow::Close() {
    if (m_window) {
        SDL_DestroyWindow(m_window);
    }
}
