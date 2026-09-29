#include "sk_engine_export.h"
#include "skylabs/engine/filesystem.hpp"
#include "skylabs/engine/os.hpp"
#include "skylabs/engine/render/vulkan/renderer.hpp"
#include "skylabs/engine/sdl/context.hpp"
#include "skylabs/engine/sdl/event_pump.hpp"
#include "skylabs/engine/sdl/filesystem.hpp"
#include "skylabs/engine/sdl/log_sink.hpp"
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

    void* context[] = { &window, &renderer };

    // Unfortunately we are locked at 64 FPS. Alternatively we can do a separate render thread
    eventPump.SetEventFilter(
        [](const sk::input::Event& event, void* userData) {
            const auto contextPtr = static_cast<void**>(userData);
            const auto windowPtr = static_cast<sk::IWindow*>(contextPtr[0]);
            const auto rendererPtr = static_cast<sk::render::IRenderer*>(contextPtr[1]);

            if (std::holds_alternative<sk::input::WindowExposeEvent>(event)) {
                if (windowPtr->IsRenderAvailable()) {
                    rendererPtr->OnPossibleSwapchainResize();
                    rendererPtr->Draw(glm::mat4(1), 0, 0);
                }
                return false;
            } else if (std::holds_alternative<sk::input::DeviceResetEvent>(event)) {
                rendererPtr->OnDeviceReset();
                return false;
            }
            return true;
        },
        &context);

    while (!quit) {
        while (auto event = eventPump.PollEvent()) {
            std::visit(
                sk::utils::Overloaded { [&](sk::input::QuitEvent) { quit = true; },
                                        [&](sk::input::DeviceResetEvent) { renderer.OnDeviceReset(); },
                                        [](auto&&) { } },
                *event);
        }

        if (window.IsRenderAvailable()) {
            renderer.Draw(glm::mat4(1), 0, 0);
        }
    }

    return 0;
}
