#pragma once
#include "skylabs/engine/camera.hpp"
#include "skylabs/engine/filesystem.hpp"
#include "skylabs/engine/render/vulkan/renderer.hpp"
#include "skylabs/engine/sdl/context.hpp"
#include "skylabs/engine/sdl/event_pump.hpp"
#include "skylabs/engine/sdl/vulkan_adapter.hpp"
#include "skylabs/engine/sdl/window.hpp"

namespace sk {
class Engine {
public:
    Engine() = default;
    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&) = delete;
    ~Engine() = default;

    void Run();

    bool OnEvent(const input::Event& event);

private:
    struct FpsCounter {
        int frames = 0;
        float timer = 0.0f;

        bool Tick(float deltaTimeMs, float& outFps, float& outDt);
    };

    float CalculateDeltaTime();
    void ReportFPS(float deltaTimeMs);
    void RenderFrame(float deltaTimeMs);

    bool m_quit = false;
    bool m_relativeMouseMode = false;
    sdl::Context m_sdlContext { nullptr };
    sdl::Window m_window { nullptr };
    sdl::EventPump m_eventPump;
    sdl::vulkan::OSAdapter m_vulkanAdapter { nullptr };

    filesystem::Filesystem m_filesystem { nullptr };

    std::optional<render::vulkan::Renderer> m_renderer;

    std::chrono::steady_clock::time_point m_lastTick;
    FpsCounter m_fpsCounter;
    Camera m_camera;
};
}
