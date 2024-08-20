#include "vulkanapi.hpp"
#include "SDL.hpp"
#include "SDL_Vulkan.hpp"

#include "console.hpp"
#include "launcher.hpp"

SDL::CWindow g_window;

void MainLoop() {
    bool quit = false;
    while (!quit) {
        SDL_Event e;
        SDL_PollEvent(&e);
        if (e.type == SDL_QUIT) {
            quit = true;
        }
    }
}

void CLauncher::Main() {
    SDL::CHandle SdlHandle;

    g_window.Create(
        "Skylabs",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_VULKAN
    );
    CVulkanAPI vulkan;
    vulkan.Init(&g_window);
    MainLoop();
}
