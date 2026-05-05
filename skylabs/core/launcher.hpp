#pragma once
#include <skylabs/public/application.hpp>
#include <skylabs/public/sdl/context.hpp>
#include <skylabs/public/sdl/ttf/context.hpp>
#include <skylabs/public/sdl/mixer/context.hpp>
#include <skylabs/public/sdl/mixer/mixer.hpp>
#include <skylabs/public/sdl/mixer/track.hpp>
#include <skylabs/public/sdl/mixer/audio.hpp>
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
    void Destroy() override;

private:
    void ProcessEvents();
    void Update(float deltaTime);
    void Render(float deltaTime);

    void HandleTouchEvent(const SDL_Event& e);
    void HandleKeyDownEvent(const SDL_KeyboardEvent& keyEvent);
    void HandleKeyUpEvent(const SDL_KeyboardEvent& keyEvent);
    void HandleTextInput(const SDL_TextInputEvent& textEvent);

    bool m_minimized = false;
    bool m_quit = false;

    bool m_textInputActive = false;
    std::string m_inputBuffer;

    Joystick m_leftJoystick;
    UIButton m_chatButton = { 0.8f, 0.05f, 0.15f, 0.1f };
    CCamera m_camera { glm::vec3(1.0f, 0.0f, 0.0f) };

    SDL::CContext m_sdlContext { nullptr };
    SDL::TTF::CContext m_sdlttfContext { nullptr };
    SDL::Mixer::CContext m_sdlmixerContext { nullptr };

    SDL::CWindow m_window { nullptr };
    std::unique_ptr<IRenderer> m_renderer { nullptr };

    SDL::Mixer::CMixer m_mixer { nullptr };
    SDL::Mixer::CTrack m_track { nullptr };
    SDL::Mixer::CAudio m_music { nullptr };
};
