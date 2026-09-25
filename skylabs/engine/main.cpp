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
    sk::sdl::Window window { "Skylabs", 640, 480, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE };
    sk::sdl::EventPump eventPump;
    bool quit = false;

    sk::filesystem::Filesystem filesystem =
        sk::filesystem::Filesystem { std::make_unique<sk::sdl::FilesystemBackend>() };

#ifdef PLATFORM_ANDROID
    log::AddSink(std::make_unique<SDL::CLogSink>());

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

    eventPump.SetEventFilter(
        [](const sk::input::Event& event, void* userData) {
            if (std::holds_alternative<sk::input::WindowExposeEvent>(event)) {
                const auto self = static_cast<sk::render::IRenderer*>(userData);
                self->OnPossibleSwapchainResize();
                self->Draw(glm::mat4(1), 0, 0)
                return false;
            }
            return true;
        },
        &renderer);

    while (!quit) {
        while (auto event = eventPump.PollEvent()) {
            std::visit(
                sk::utils::Overloaded { [&](sk::input::QuitEvent) { quit = true; }, [](auto&&) { } },
                *event);
        }
    }

    return 0;
}
