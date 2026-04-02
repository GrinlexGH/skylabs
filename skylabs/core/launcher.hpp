#pragma once
#include <skylabs/public/application.hpp>
#include <skylabs/public/sdl/context.hpp>
#include <skylabs/public/sdl/window.hpp>
#include <skylabs/core/render/renderer.hpp>
#include <skylabs/core/camera.hpp>

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

class CLauncher final : public CBaseApplication
{
public:
    void Create() override;
    void Main() override;

private:
    void Initialize();

    void ProcessEvents();
    void Update(float deltaTime);
    void Render(float deltaTime);

    void HandleWindowEvent(const SDL_Event& e);
    void HandleTouchEvent(const SDL_Event& e);
    void HandleMouseEvent(const SDL_Event& e);
    void HandleKeyboardEvent(const SDL_KeyboardEvent& keyEvent, bool isDown);
    void HandleTextInput(const SDL_TextInputEvent& textEvent);

    void Pause() { m_paused = true; }
    void Resume() { m_paused = false; }

    bool m_quit = false;
    bool m_paused = false;

    bool m_textInputActive = false;
    std::string m_inputBuffer;

    Joystick m_leftJoystick;
    UIButton m_chatButton = { 0.8f, 0.05f, 0.15f, 0.1f };
    CCamera m_camera { glm::vec3(1.0f, 0.0f, 0.0f) };

    SDL::CContext m_sdlContext { nullptr };
    std::unique_ptr<IRenderer> m_renderer { nullptr };
    SDL::CWindow m_window { nullptr };
};
