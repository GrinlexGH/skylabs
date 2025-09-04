#include "launcher.hpp"

#include "SDL/context.hpp"
#include "SDL/vulkan/window.hpp"
#include "camera.hpp"
#include "render/vulkan/renderer.hpp"

CCamera g_camera { glm::vec3(1.0f, 0.0f, 0.0f) };

void MainLoop(const std::unique_ptr<IRenderer>& renderer, SDL_Window* window) {
    bool quit = false;
    while (!quit) {
        static float deltaTime = 0.0f;
        static float lastFrame = 0.0f;

        const Uint64 currentFrame = SDL_GetTicks();
        deltaTime = (static_cast<float>(currentFrame) - lastFrame) / 1000.0f;
        lastFrame = static_cast<float>(currentFrame);

        static int frameCount = 0;
        static float elapsedTime = 0.0f;

        frameCount++;
        elapsedTime += deltaTime;

        if (elapsedTime >= 1.0f) {
            const float fps = static_cast<float>(frameCount) / elapsedTime;

            const std::string title = "Skylabs | FPS: " + std::to_string(static_cast<int>(fps));
            SDL_SetWindowTitle(window, title.c_str());

            frameCount = 0;
            elapsedTime = 0.0f;
        }

        static bool minimized = false;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
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
                    // renderer.m_frameBufferResized = true;
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    g_camera.ProcessMouseMovement(e.motion.xrel, -e.motion.yrel);
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    g_camera.ProcessMouseScroll(e.wheel.y);
                    break;
                case SDL_EVENT_KEY_DOWN:
                    switch (e.key.key) {
                        case SDLK_ESCAPE: {
                            quit = true;
                        } break;
                        case SDLK_P: {
                            static bool mouseModeSwitch = false;
                            SDL_SetWindowRelativeMouseMode(window, mouseModeSwitch);
                            mouseModeSwitch = !mouseModeSwitch;
                        } break;
                        case SDLK_F11: {
                            static bool fullscreenSwitch = true;
                            SDL_SetWindowFullscreen(window, fullscreenSwitch);
                            fullscreenSwitch = !fullscreenSwitch;
                        } break;
                        case SDLK_W: {
                            g_camera.ProcessKeyboard(FORWARD, deltaTime);
                        } break;
                        case SDLK_A: {
                            g_camera.ProcessKeyboard(LEFT, deltaTime);
                        } break;
                        case SDLK_S: {
                            g_camera.ProcessKeyboard(BACKWARD, deltaTime);
                        } break;
                        case SDLK_D: {
                            g_camera.ProcessKeyboard(RIGHT, deltaTime);
                        } break;
                        case SDLK_LSHIFT: {
                            g_camera.MoveFaster();
                        } break;
                        case SDLK_UP: {
                            MoveForward();
                        } break;
                        case SDLK_DOWN: {
                            MoveBackward();
                        } break;
                        default:
                            break;
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    switch (e.key.key) {
                        case SDLK_LSHIFT: {
                            g_camera.ResetSpeed();
                        } break;
                        default:
                            break;
                    }
                    break;
                default: break;
            }
        }

        const bool* keyboardState = SDL_GetKeyboardState(nullptr);

        if (keyboardState[SDL_SCANCODE_W]) {
            g_camera.ProcessKeyboard(FORWARD, deltaTime);
        }
        if (keyboardState[SDL_SCANCODE_A]) {
            g_camera.ProcessKeyboard(LEFT, deltaTime);
        }
        if (keyboardState[SDL_SCANCODE_S]) {
            g_camera.ProcessKeyboard(BACKWARD, deltaTime);
        }
        if (keyboardState[SDL_SCANCODE_D]) {
            g_camera.ProcessKeyboard(RIGHT, deltaTime);
        }

        if (!minimized) {
            renderer->Draw(g_camera.GetViewMatrix(), deltaTime);
        }
    }
}

void CLauncher::Main() {
    const SDL::CContext sdl(SDL_INIT_VIDEO);

    const SDL::Vulkan::CWindow window("Skylabs", 640, 480, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowRelativeMouseMode(window.GetHandle(), true);

    const std::unique_ptr<IRenderer> renderer = Vulkan::CRenderer::TryToCreate(&window);
    if (!renderer) {
        throw std::runtime_error("Cannot initialize vulkan!\n");
    }

    MainLoop(renderer, window.GetHandle());
}
