#include "vulkanapi.hpp"
#include "SDL.hpp"

#include "launcher.hpp"

SDL::CWindow g_window;

void MainLoop(CVulkanRenderer& renderer) {
    bool quit = false;
    bool minimized = false;
    while (!quit) {
        SDL_Event e;
        SDL_PollEvent(&e);
        switch (e.type) {
            case SDL_EVENT_QUIT:
                quit = true;
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                minimized = true;
                break;
            case SDL_EVENT_WINDOW_RESTORED:
                minimized = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                renderer.m_frameBufferResized = true;
                break;
        }
        if(!minimized) {
            renderer.Draw();
        }
    }
}

void CLauncher::Main() {
    SDL::CHandle SdlHandle;
    g_window.Create(
        "Skylabs",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );
    CVulkanRenderer vulkan;
    vulkan.Init(&g_window);
    MainLoop(vulkan);
}
