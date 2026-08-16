#pragma once
#include <skylabs/core/camera.hpp>
#include <skylabs/core/render/renderer.hpp>
#include <skylabs/core/render/vertex.hpp>
#include <skylabs/core/render/vulkan/render_object.hpp>
#include <skylabs/core/render/vulkan/renderer.hpp>
#include <skylabs/public/application.hpp>
#include <skylabs/public/sdl/context.hpp>
#include <skylabs/public/sdl/event_pump.hpp>
#include <skylabs/public/sdl/sdl.hpp>
#include <skylabs/public/sdl/vulkan/os_connector.hpp>
#include <skylabs/public/sdl/window.hpp>

struct Joystick
{
    bool active = false;
    SDL_FingerID fingerId = 0;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float dirX = 0.0f;
    float dirY = 0.0f;
    static constexpr float radius = 0.15f;
};

struct UIButton
{
    float x, y, w, h;

    bool IsInside(float touchX, float touchY) {
        return (touchX >= x && touchX <= x + w && touchY >= y && touchY <= y + h);
    }
};

struct SDisk {
    int size;
    bool isselected;
    Vulkan::CRenderObject renderObject;
};

struct STower {
    glm::vec3 basePosition;
    Vulkan::CRenderObject stemRenderObject;
    std::vector<SDisk> disks;
    std::uint16_t baseColorId;
};

class CLauncher final : public CBaseApplication
{
public:
    void Create() override;
    void Main() override;
    void Destroy() override;

private:
    void ProcessEvents();
    void Update(float deltaTime);
    void Render(float deltaTime);
    void UpdateVisuals(float deltaTime);
    void Click();

    void HandleKeyDownEvent(const Keys& key);
    void HandleKeyUpEvent(const Keys& key);
    void HandleTextInput(const SDL_TextInputEvent& textEvent);

    void OnQuit(QuitEvent) { m_quit = true; }
    void OnKeyEvent(KeyEvent e) { if (e.down) HandleKeyDownEvent(e.key); else HandleKeyUpEvent(e.key); }
    void OnDeviceResetEvent(DeviceResetEvent) { m_renderer->OnDeviceLost(); }
    void OnMouseMotionEvent(MouseMotionEvent e) { m_camera.ProcessMouseMovement(e.dx, -e.dy); }
    void OnMouseWheelEvent(MouseWheelEvent e) { m_camera.ProcessMouseScroll(e.y); }
    void OnMouseButtonEvent(MouseButtonEvent e) { if (e.down) Click(); }
    void OnFingerTouchEvent(const FingerTouchEvent& e);
    void OnFingerMotionEvent(const FingerMotionEvent& e);

    static bool Watcher(void* userdata, SDL_Event* event);

    std::tuple<std::vector<CVertex>, std::vector<std::uint16_t>> GenerateDisk();

    bool m_quit = false;

    entt::dispatcher m_eventDispatcher;

    bool m_textInputActive = false;
    std::string m_inputBuffer;

    Joystick m_leftJoystick;
    UIButton m_chatButton = { 0.8f, 0.05f, 0.15f, 0.1f };
    CCamera m_camera { glm::vec3(0.0f, 0.0f, 0.0f) };

    SDL::CContext m_sdlContext { nullptr };

    SDL::CWindow m_window { nullptr };
    SDL::Vulkan::COSConnector m_surfaceProvider { nullptr };
    SDL::CEventPump m_eventPump;
    std::unique_ptr<Vulkan::CRenderer> m_renderer { nullptr };

    std::vector<STower> m_towers;
    int m_hoveredTowerIdx = -1;
    std::tuple<int, int> m_towerDiskselecte { -1, -1 };
};
