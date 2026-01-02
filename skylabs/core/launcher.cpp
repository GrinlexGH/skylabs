#include <skylabs/core/launcher.hpp>

#include <skylabs/core/SDL/context.hpp>
#include <skylabs/core/SDL/keyboard.hpp>
#include <skylabs/core/SDL/vulkan/window.hpp>
#include <skylabs/core/camera.hpp>
#include <skylabs/core/render/vulkan/renderer.hpp>

#include <steam/steam_api.h>

#include <span>

CCamera g_camera { glm::vec3(1.0f, 0.0f, 0.0f) };

void MainLoop(const std::unique_ptr<IRenderer>& renderer, const SDL::Vulkan::CWindow& window) {
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
                    renderer->SetResizedState(true);
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
                            SDL_SetWindowRelativeMouseMode(*window, mouseModeSwitch);
                            mouseModeSwitch = !mouseModeSwitch;
                        } break;
                        case SDLK_F11: {
                            static bool fullscreenSwitch = true;
                            SDL_SetWindowFullscreen(*window, fullscreenSwitch);
                            fullscreenSwitch = !fullscreenSwitch;
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

        const std::span keyboardState = SDL::GetKeyboardState();

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
        if (keyboardState[SDL_SCANCODE_LSHIFT]) {
            g_camera.MoveFaster();
        }

        if (!minimized) {
            renderer->Draw(g_camera.GetViewMatrix(), deltaTime);
        }
    }
    SteamAPI_RunCallbacks();
}

// #define ENABLE_BENCHMARKS
#include <benchmark/benchmark.h>

#ifdef ENABLE_BENCHMARKS
namespace {
void BM_CreateInstance(benchmark::State& state) {
    for (auto _ : state) {
        Vulkan::CInstance instance({}, {});
    }
}
}

BENCHMARK(BM_CreateInstance)->Unit(benchmark::kMillisecond)->Iterations(50);
#endif

void CLauncher::Main() {
#ifdef ENABLE_BENCHMARKS
    int argc = 0;
    char** argv = nullptr;
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
#endif

    const SDL::CContext sdl(SDL_INIT_VIDEO);

    const SDL::Vulkan::CWindow window("Skylabs", 640, 480, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowRelativeMouseMode(window.GetHandle(), true);

    const std::unique_ptr<IRenderer> renderer = Vulkan::CRenderer::TryToCreate(&window);
    if (!renderer) {
        throw std::runtime_error { "Cannot initialize vulkan!\n" };
    }

    MainLoop(renderer, window);
}
