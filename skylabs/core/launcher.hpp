#pragma once
#include <chrono>

#include <SDL3/SDL.h>
#include <entt/entt.hpp>

#include "../engine/filesystem.hpp"
#include "skylabs/base/application.hpp"
#include "skylabs/base/sdl/context.hpp"
#include "skylabs/base/sdl/event_pump.hpp"
#include "skylabs/base/sdl/vulkan/os_connector.hpp"
#include "skylabs/base/sdl/window.hpp"
#include "skylabs/core/camera.hpp"
#include "skylabs/core/render/vertex.hpp"
#include "skylabs/core/render/vulkan/render_object.hpp"
#include "skylabs/core/render/vulkan/renderer.hpp"

namespace sk {
struct Joystick {
    bool active = false;
    SDL_FingerID fingerId = 0;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float dirX = 0.0f;
    float dirY = 0.0f;
    static constexpr float kRadius = 0.15f;
};

struct UIButton {
    float x, y, w, h;

    bool IsInside(float touchX, float touchY) const {
        return (touchX >= x && touchX <= x + w && touchY >= y && touchY <= y + h);
    }
};

struct Disk {
    int size;
    bool isselected;
    render::vulkan::RenderObject renderObject;
};

struct Tower {
    glm::vec3 basePosition;
    render::vulkan::RenderObject stemRenderObject;
    std::vector<Disk> disks;
    std::uint16_t baseColorId;
};

class Launcher final : public BaseApplication {
public:
    void PreCreate() override;
    void Create() override;
    void Main() override;
    void Destroy() override;

private:
    void ProcessEvents();
    void LoopIteration();
    void Update(float deltaTime);
    void Render(float deltaTime);
    void UpdateVisuals(float deltaTime);
    void Click();

    void HandleKeyDownEvent(input::Keys key);
    void HandleKeyUpEvent(input::Keys key);
    void HandleTextInput(const SDL_TextInputEvent& textEvent);

    void OnQuit(input::QuitEvent) { m_quit = true; }
    void OnKeyEvent(input::KeyEvent e) {
        if (e.down)
            HandleKeyDownEvent(e.key);
        else
            HandleKeyUpEvent(e.key);
    }

    void OnDeviceResetEvent(input::DeviceResetEvent) { m_renderer->OnDeviceLost(); }
    void OnMouseMotionEvent(input::MouseMotionEvent e) {
        m_camera.ProcessMouseMovement(e.dx, -e.dy);
    }
    void OnMouseWheelEvent(input::MouseWheelEvent e) { m_camera.ProcessMouseScroll(e.y); }
    void OnMouseButtonEvent(input::MouseButtonEvent e) {
        if (e.down) Click();
    }

    void OnFingerTouchEvent(const input::FingerTouchEvent e);
    void OnFingerMotionEvent(const input::FingerMotionEvent& e);

    static bool Watcher(void* userdata, SDL_Event* event);

    void InitFilesystem();

    std::tuple<std::vector<Vertex>, std::vector<std::uint16_t>> GenerateDisk();

    constexpr static int kTargetFps = 60;
    constexpr static int kFrameDelay = 1000 / kTargetFps;

    bool m_quit = false;
    int m_frameCount = 0;
    float m_elapsedTime = 0.0f;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTick =
        std::chrono::high_resolution_clock::now();

    entt::dispatcher m_eventDispatcher;

    bool m_textInputActive = false;
    std::string m_inputBuffer;

    Joystick m_leftJoystick;
    UIButton m_chatButton = { 0.8f, 0.05f, 0.15f, 0.1f };
    Camera m_camera { glm::vec3(0.0f, 0.0f, 0.0f) };

    filesystem::Filesystem m_filesystem { nullptr };

    sdl::Context m_sdlContext { nullptr };
    sdl::Window m_window { nullptr };
    sdl::EventPump m_eventPump;
    sdl::vulkan::OSConnector m_osConnector { nullptr };
    std::optional<render::vulkan::Renderer> m_renderer;

    std::vector<Tower> m_towers;
    int m_hoveredTowerIdx = -1;
    std::tuple<int, int> m_selectedTowerAndDisk { -1, -1 };
};
}
