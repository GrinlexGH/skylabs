#include "sk_engine_export.h"
#include "skylabs/engine/camera.hpp"
#include "skylabs/engine/filesystem.hpp"
#include "skylabs/engine/os.hpp"
#include "skylabs/engine/render/vulkan/renderer.hpp"
#include "skylabs/engine/sdl/context.hpp"
#include "skylabs/engine/sdl/event_pump.hpp"
#include "skylabs/engine/sdl/filesystem.hpp"
#include "skylabs/engine/sdl/log_sink.hpp"
#include "skylabs/engine/sdl/sdl.hpp"
#include "skylabs/engine/sdl/vulkan_adapter.hpp"
#include "skylabs/engine/sdl/window.hpp"

SK_ENGINE_PUBLIC_INTERFACE int SkMain(int /*argc*/, char* /*argv*/[]) {
    sk::sdl::Context sdlContext { SDL_INIT_VIDEO };
    sk::sdl::Window window { "Skylabs", 640, 480,
                             SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
#ifdef PLATFORM_ANDROID
                                 | SDL_WINDOW_FULLSCREEN
#endif
    };

    sk::sdl::EventPump eventPump;
    bool quit = false;

    sk::filesystem::Filesystem filesystem =
        sk::filesystem::Filesystem { std::make_unique<sk::sdl::FilesystemBackend>() };

#ifdef PLATFORM_ANDROID
    sk::log::AddSink(std::make_unique<sk::sdl::LogSink>());

    filesystem.Mount("assets", "");
    filesystem.Mount("assets", "assets:/");
    filesystem.Mount("res", "");
#else
    sk::log::AddSink(std::make_unique<sk::log::ConsoleSink>());

    filesystem.Mount("assets", sk::os::JoinPath(sk::os::GetAppRoot(), "assets"));
    filesystem.Mount("assets", sk::os::GetAppRoot());
    filesystem.Mount("res", sk::os::GetAppRoot());
#endif

    sk::sdl::vulkan::OSAdapter vulkanAdapter { *window };
    sk::render::vulkan::Renderer renderer { &window, &vulkanAdapter, filesystem };

    sk::Camera camera;

    int frameCount = 0;
    float elapsedTime = 0.0f;
    auto lastTick = std::chrono::high_resolution_clock::now();

    void* context[] = { &window, &renderer, &lastTick, &camera };

    // Unfortunately we are locked at 64 FPS. Alternatively we can do a separate render thread
    eventPump.SetEventFilter(
        [](const sk::input::Event& event, void* userData) {
            const auto contextPtr = static_cast<void**>(userData);
            const auto windowPtr = static_cast<sk::IWindow*>(contextPtr[0]);
            const auto rendererPtr = static_cast<sk::render::IRenderer*>(contextPtr[1]);
            const auto lastTickPtr = static_cast<std::chrono::steady_clock::time_point*>(contextPtr[2]);
            const auto cameraPtr = static_cast<sk::Camera*>(contextPtr[3]);

            if (std::holds_alternative<sk::input::WindowExposeEvent>(event)) {
                const auto frameStart = std::chrono::high_resolution_clock::now();
                const float deltaTimeMs =
                    std::chrono::duration<float, std::milli>(frameStart - *lastTickPtr).count();
                *lastTickPtr = frameStart;

                if (windowPtr->IsRenderAvailable()) {
                    rendererPtr->OnPossibleSwapchainResize();
                    rendererPtr->Draw(cameraPtr->ViewMatrix(), cameraPtr->Fov(), deltaTimeMs);
                }
                return false;
            }

            if (std::holds_alternative<sk::input::DeviceResetEvent>(event)) {
                rendererPtr->OnDeviceReset();
                return false;
            }

            return true;
        },
        &context);

    bool relativeMouseMode = false;

    while (!quit) {
        const auto frameStart = std::chrono::high_resolution_clock::now();
        const float deltaTimeMs =
            std::chrono::duration<float, std::milli>(frameStart - lastTick).count();
        lastTick = frameStart;

        while (auto event = eventPump.PollEvent()) {
            std::visit(sk::utils::Overloaded {
                           [&](sk::input::QuitEvent) { quit = true; },
                           [&](sk::input::DeviceResetEvent) { renderer.OnDeviceReset(); },
                           [&](const sk::input::MouseMotionEvent& e) {
                               camera.ProcessMouseMovement(e.dx, -e.dy);
                           },
                           [&](const sk::input::KeyEvent e) {
                               if (!e.down) return;

                               switch (e.key) {
                                   case sk::input::Keys::eEscape:
                                       quit = true;
                                       break;
                                   case sk::input::Keys::eZ:
                                       SDL_SetWindowRelativeMouseMode(*window, relativeMouseMode);
                                       relativeMouseMode = !relativeMouseMode;
                                       break;

                                   default:
                                       break;
                               }
                           },
                           [](const auto&) { } },
                       *event);
        }

        const std::span keyboardState = sk::sdl::GetKeyboardState();
        if (keyboardState[SDL_SCANCODE_W])
            camera.ProcessKeyboard(sk::Camera::MoveDirection::eForward, deltaTimeMs);
        if (keyboardState[SDL_SCANCODE_S])
            camera.ProcessKeyboard(sk::Camera::MoveDirection::eBackward, deltaTimeMs);
        if (keyboardState[SDL_SCANCODE_A])
            camera.ProcessKeyboard(sk::Camera::MoveDirection::eLeft, deltaTimeMs);
        if (keyboardState[SDL_SCANCODE_D])
            camera.ProcessKeyboard(sk::Camera::MoveDirection::eRight, deltaTimeMs);

        if (window.IsRenderAvailable()) {
            renderer.Draw(camera.ViewMatrix(), camera.Fov(), 0);
        }

        frameCount++;
        elapsedTime += deltaTimeMs;
        if (elapsedTime >= 1000.0f) {
            float avgFps = frameCount * (1000.0f / elapsedTime);
            float avgDt = elapsedTime / static_cast<float>(frameCount);
            std::string title = fmt::format("Skylabs | FPS: {:.0f} | DT: {:.2f}ms", avgFps, avgDt);
            SDL_SetWindowTitle(*window, title.c_str());
            sk::log::Debug("{}", title);
            elapsedTime = 0.0f;
            frameCount = 0;
        }
    }

    return 0;
}
