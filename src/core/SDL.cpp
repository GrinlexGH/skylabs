#include "SDL.hpp"
#include <stdexcept>

//============
// SDL Handle
unsigned int SDL::Handle::instanceCount_ = 0;

SDL::Handle::Handle() {
    if (instanceCount_ == 0) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            throw std::runtime_error(
                std::string { "Couldn't initialize SDL!" } + SDL_GetError()
            );
        }
    }

    ++instanceCount_;
}

SDL::Handle::~Handle() {
    --instanceCount_;
    if (instanceCount_ == 0) {
        SDL_Quit();
    }
}

//============
// SDL Window
SDL::CWindow::CWindow(const char* title, int x, int y, int w, int h, unsigned int SDLflags) {
    Create(title, x, y, w, h, SDLflags);
}

void SDL::CWindow::Create(const char* title, int x, int y, int w, int h, unsigned int SDLflags) {
    if (window_)
        return;

    if (SDL::Handle::InstanceCount() == 0) {
        throw std::runtime_error(
            "You must create SDL::Handle object before using this function!"
        );
    }

    window_ = SDL_CreateWindow(title, x, y, w, h, SDLflags);
}

SDL::CWindow::~CWindow() {
    Close();
}

void SDL::CWindow::Close() {
    if (window_) {
        SDL_DestroyWindow(window_);
    }
}
