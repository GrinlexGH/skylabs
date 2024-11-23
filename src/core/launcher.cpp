#include "launcher.hpp"

#include "render/vulkan_renderer.hpp"
#include "SDL/SDL.hpp"
#include "console.hpp"
#include "camera.hpp"

CCamera g_camera {glm::vec3(0.0f, 0.0f, 0.0f)};

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = 640 / 2.0f;
float lastY = 480 / 2.0f;
bool firstMouse = true;

void MainLoop(CVulkanRenderer& renderer) {
    bool quit = false;
    bool minimized = false;
    while (!quit) {
        Uint64 currentFrame = SDL_GetTicks();
        deltaTime = currentFrame - lastFrame;
        lastFrame = (float)currentFrame;

        const bool* keyState = SDL_GetKeyboardState(nullptr);
        if (keyState[SDL_SCANCODE_W]) {
            g_camera.ProcessKeyboard(FORWARD, deltaTime);
        }
        if (keyState[SDL_SCANCODE_A]) {
            g_camera.ProcessKeyboard(LEFT, deltaTime);
        }
        if (keyState[SDL_SCANCODE_S]) {
            g_camera.ProcessKeyboard(BACKWARD, deltaTime);
        }
        if (keyState[SDL_SCANCODE_D]) {
            g_camera.ProcessKeyboard(RIGHT, deltaTime);
        }

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
            case SDL_EVENT_MOUSE_MOTION:
                g_camera.ProcessMouseMovement(e.motion.xrel, -e.motion.yrel);
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                g_camera.ProcessMouseScroll(e.wheel.y);
                break;
        }
        if(!minimized) {
            renderer.Draw();
        }
    }
}

void CLauncher::Main() {
    SDL::CContext context(SDL_INIT_VIDEO);
    SDL::CVulkanWindow window("Skylabs", 640, 480, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowRelativeMouseMode(window.m_window, true);

    CVulkanRenderer vulkan;
    if (!vulkan.Initialize(&window)) {
        throw std::runtime_error("Cannot initialize vulkan!\n");
    }
    MainLoop(vulkan);
}
