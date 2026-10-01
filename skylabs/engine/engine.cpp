#include "skylabs/engine/engine.hpp"
#include "skylabs/engine/os.hpp"
#include "skylabs/engine/sdl/filesystem.hpp"
#include "skylabs/engine/sdl/log_sink.hpp"
#include "skylabs/engine/sdl/sdl.hpp"

namespace sk {
void Engine::Run() {
    m_sdlContext = sdl::Context { SDL_INIT_VIDEO };
    m_window = sdl::Window { "Skylabs", 640, 480,
                             SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
#ifdef PLATFORM_ANDROID
                                 | SDL_WINDOW_FULLSCREEN
#endif
    };

    m_filesystem = filesystem::Filesystem { std::make_unique<sdl::FilesystemBackend>() };

#ifdef PLATFORM_ANDROID
    log::AddSink(std::make_unique<sdl::LogSink>());

    m_filesystem.Mount("assets", "");
    m_filesystem.Mount("assets", "assets:/");
    m_filesystem.Mount("res", "");
#else
    log::AddSink(std::make_unique<log::ConsoleSink>());

    m_filesystem.Mount("assets", os::JoinPath(os::GetAppRoot(), "assets"));
    m_filesystem.Mount("assets", os::GetAppRoot());
    m_filesystem.Mount("res", os::GetAppRoot());
#endif

    m_vulkanAdapter = sdl::vulkan::OSAdapter { *m_window };
    m_renderer.emplace(&m_window, &m_vulkanAdapter, m_filesystem);

    // Unfortunately we are locked at 64 FPS. Alternatively we can do a separate render thread
    m_eventPump.SetEventFilter(
        [](const input::Event& event, void* userData) {
            const auto engine = static_cast<Engine*>(userData);
            return engine->OnEvent(event);
        },
        this);

    m_lastTick = std::chrono::steady_clock::now();

    while (!m_quit) {
        const float deltaTimeMs = CalculateDeltaTime();

        while (auto event = m_eventPump.PollEvent()) {
            OnEvent(*event);
        }

        const std::span keyboardState = sdl::GetKeyboardState();
        if (keyboardState[SDL_SCANCODE_W])
            m_camera.ProcessKeyboard(Camera::MoveDirection::eForward, deltaTimeMs);
        if (keyboardState[SDL_SCANCODE_S])
            m_camera.ProcessKeyboard(Camera::MoveDirection::eBackward, deltaTimeMs);
        if (keyboardState[SDL_SCANCODE_A])
            m_camera.ProcessKeyboard(Camera::MoveDirection::eLeft, deltaTimeMs);
        if (keyboardState[SDL_SCANCODE_D])
            m_camera.ProcessKeyboard(Camera::MoveDirection::eRight, deltaTimeMs);

        if (m_window.IsRenderAvailable()) {
            m_renderer->Draw(m_camera.ViewMatrix(), m_camera.Fov(), deltaTimeMs);
        }

        ReportFPS(deltaTimeMs);
    }
}

bool Engine::OnEvent(const input::Event& event) {
    return event | utils::Overloaded { [&](input::QuitEvent) {
                                          m_quit = true;
                                          return false;
                                      },
                                       [&](input::DeviceResetEvent) {
                                           m_renderer->OnDeviceReset();
                                           return false;
                                       },
                                       [&](const input::MouseMotionEvent& e) {
                                           m_camera.ProcessMouseMovement(e.dx, -e.dy);
                                           return false;
                                       },
                                       [&](const input::KeyEvent e) {
                                           if (!e.down) return true;

                                           switch (e.key) {
                                               case input::Keys::eEscape:
                                                   m_quit = true;
                                                   break;

                                               case input::Keys::eZ:
                                                   m_relativeMouseMode = !m_relativeMouseMode;
                                                   SDL_SetWindowRelativeMouseMode(*m_window,
                                                                                  m_relativeMouseMode);
                                                   break;

                                               default:
                                                   break;
                                           }
                                           return false;
                                       },
                                       [&](input::WindowExposeEvent) {
                                           RenderFrame(CalculateDeltaTime());
                                           return false;
                                       },
                                       [](const auto&) { return true; } };
}

bool Engine::FpsCounter::Tick(const float deltaTimeMs, float& outFps, float& outDt) {
    frames++;
    timer += deltaTimeMs;
    if (timer >= 1000.0f) {
        outFps = static_cast<float>(frames) * (1000.0f / timer);
        outDt = timer / static_cast<float>(frames);
        timer -= 1000.0f;
        frames = 0;
        return true;
    }
    return false;
}

float Engine::CalculateDeltaTime() {
    const auto frameStart = std::chrono::steady_clock::now();
    const float deltaTimeMs = std::chrono::duration<float, std::milli>(frameStart - m_lastTick).count();
    m_lastTick = frameStart;
    return deltaTimeMs;
}

void Engine::ReportFPS(const float deltaTimeMs) {
    if (float avgFps, avgDt; m_fpsCounter.Tick(deltaTimeMs, avgFps, avgDt)) {
        std::string title = fmt::format("Skylabs | FPS: {:.0f} | DT: {:.2f}ms", avgFps, avgDt);
        SDL_SetWindowTitle(*m_window, title.c_str());
        log::Debug("{}", title);
    }
}

void Engine::RenderFrame(const float deltaTimeMs) {
    if (m_window.IsRenderAvailable()) {
        m_renderer->OnPossibleSwapchainResize();
        m_renderer->Draw(m_camera.ViewMatrix(), m_camera.Fov(), deltaTimeMs);
    }

    ReportFPS(deltaTimeMs);
}
}
