#include <skylabs/core/launcher.hpp>

#include <skylabs/public/sdl/context.hpp>
#include <skylabs/public/sdl/window.hpp>
#include <skylabs/public/sdl/sdl.hpp>
#include <skylabs/public/logging.hpp>
#include <skylabs/core/camera.hpp>
#include <skylabs/core/render/vulkan/renderer.hpp>

#include <steam/steam_api.h>

#include <span>

struct Joystick {
    bool active = false;
    SDL_FingerID fingerId = 0;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float dirX = 0.0f;
    float dirY = 0.0f;
    static constexpr float radius = 0.15f;
} g_leftJoystick;

struct UIButton {
    float x, y, w, h;
    bool IsInside(float touchX, float touchY) {
        return (touchX >= x && touchX <= x + w && touchY >= y && touchY <= y + h);
    }
} g_chatButton = { 0.8f, 0.05f, 0.15f, 0.1f };

CCamera g_camera { glm::vec3(1.0f, 0.0f, 0.0f) };

void MainLoop(const std::unique_ptr<IRenderer>& renderer, const SDL::CWindow& window) {
    bool textInputActive = false;
    std::string inputBuffer;

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
            SDL_SetWindowTitle(*window, title.c_str());
            Log::Debug("{}", title);

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
#ifdef PLATFORM_ANDROID
                case SDL_EVENT_WINDOW_FOCUS_LOST:
#endif
                case SDL_EVENT_WINDOW_MINIMIZED:
                    minimized = true;
                    break;
#ifdef PLATFORM_ANDROID
                case SDL_EVENT_WINDOW_FOCUS_GAINED:
#endif
                case SDL_EVENT_WINDOW_RESTORED:
                    minimized = false;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    renderer->m_isResized = true;
                    break;
                case SDL_EVENT_TEXT_INPUT: {
                    inputBuffer += e.text.text;
                    Log::Info("[Input] Current buffer: {}", inputBuffer);
                } break;
                case SDL_EVENT_FINGER_DOWN: {
                    if (g_chatButton.IsInside(e.tfinger.x, e.tfinger.y)) {
                        textInputActive = !textInputActive;
                        if (textInputActive) {
                            SDL_StartTextInput(*window);
                        } else {
                            SDL_StopTextInput(*window);
                        }
                    } else if (e.tfinger.x < 0.5f && !g_leftJoystick.active) {
                        g_leftJoystick.active = true;
                        g_leftJoystick.fingerId = e.tfinger.fingerID;
                        g_leftJoystick.centerX = e.tfinger.x;
                        g_leftJoystick.centerY = e.tfinger.y;
                    }
                    break;
                }
                case SDL_EVENT_FINGER_MOTION: {
                    if (g_leftJoystick.active && e.tfinger.fingerID == g_leftJoystick.fingerId) {
                        float dx = e.tfinger.x - g_leftJoystick.centerX;
                        float dy = e.tfinger.y - g_leftJoystick.centerY;

                        float dist = std::sqrt(dx * dx + dy * dy);
                        if (dist > 0.001f) {
                            float scale = (dist > g_leftJoystick.radius) ? (g_leftJoystick.radius / dist) : 1.0f;
                            g_leftJoystick.dirX = (dx * scale) / g_leftJoystick.radius;
                            g_leftJoystick.dirY = (dy * scale) / g_leftJoystick.radius;
                        }
                    } else {
                        int w, h;
                        SDL_GetWindowSize(*window, &w, &h);
                        g_camera.ProcessMouseMovement(e.tfinger.dx * w, -e.tfinger.dy * h);
                    }
                    break;
                }
                case SDL_EVENT_FINGER_UP: {
                    if (g_leftJoystick.active && e.tfinger.fingerID == g_leftJoystick.fingerId) {
                        g_leftJoystick.active = false;
                        g_leftJoystick.dirX = 0.0f;
                        g_leftJoystick.dirY = 0.0f;
                    }
                    break;
                }
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
                        case SDLK_RETURN: {
                            textInputActive = false;
                            SDL_StopTextInput(*window);
                            Log::Debug("Keyboard Closed. Final text: {}", inputBuffer);
                            inputBuffer.clear();
                        } break;
                        case SDLK_P: {
                            static bool mouseModeSwitch = false;
                            SDL_SetWindowRelativeMouseMode(*window, mouseModeSwitch);
                            mouseModeSwitch = !mouseModeSwitch;
                        } break;
                        case SDLK_F11: {
                            static bool fullscreenSwitch = true;
                            SDL_SetWindowFullscreen(*window, fullscreenSwitch);
                            fullscreenSwitch = !fullscreenSwitch;
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

        const std::span keyboardState = SDL::GetKeyboardState();

        if (g_leftJoystick.active) {
            if (std::abs(g_leftJoystick.dirY) > 0.1f) {
                auto direction = (g_leftJoystick.dirY < 0) ? FORWARD : BACKWARD;
                g_camera.ProcessKeyboard(direction, deltaTime * std::abs(g_leftJoystick.dirY));
            }

            if (std::abs(g_leftJoystick.dirX) > 0.1f) {
                auto direction = (g_leftJoystick.dirX < 0) ? LEFT : RIGHT;
                g_camera.ProcessKeyboard(direction, deltaTime * std::abs(g_leftJoystick.dirX));
            }
        } else {
            if (keyboardState[SDL_SCANCODE_W]) g_camera.ProcessKeyboard(FORWARD, deltaTime);
            if (keyboardState[SDL_SCANCODE_S]) g_camera.ProcessKeyboard(BACKWARD, deltaTime);
            if (keyboardState[SDL_SCANCODE_A]) g_camera.ProcessKeyboard(LEFT, deltaTime);
            if (keyboardState[SDL_SCANCODE_D]) g_camera.ProcessKeyboard(RIGHT, deltaTime);
        }

        if (!minimized) {
            renderer->Draw(g_camera.GetViewMatrix(), deltaTime);
        }
    }
}

// #define ENABLE_BENCHMARKS

#ifdef ENABLE_BENCHMARKS
#include <catch2/catch_all.hpp>

TEST_CASE("Vulkan Performance", "[benchmark]") {
    const SDL::Vulkan::CWindow window("Skylabs", 640, 480, SDL_WINDOW_RESIZABLE);

    BENCHMARK("CreateInstance") {
        return Vulkan::CContext { &window };
    };
}
#endif

void CLauncher::Main() {
    const SDL::CContext sdl(SDL_INIT_VIDEO);

#ifdef ENABLE_BENCHMARKS
    Catch::Session().run();
#endif

    const SDL::CWindow window("Skylabs", 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    SDL_SetWindowRelativeMouseMode(*window, true);

#ifdef PLATFORM_ANDROID
    SDL_SetWindowFullscreen(*window, true);
#endif

    const std::unique_ptr<IRenderer> renderer = Vulkan::CRenderer::TryToCreate(&window);
    if (!renderer) {
        throw std::runtime_error { "Cannot initialize vulkan!\n" };
    }

    MainLoop(renderer, window);
}
