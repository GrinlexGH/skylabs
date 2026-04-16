#include <skylabs/core/launcher.hpp>
#include <skylabs/core/render/vulkan/renderer.hpp>
#include <skylabs/public/logging.hpp>
#include <skylabs/public/sdl/sdl.hpp>
#include <skylabs/public/sdl/filesystem.hpp>

#include <SDL3/SDL_system.h>

#include <span>
#include <thread>

namespace {
bool SDLCALL HandleAppEvents(void*, SDL_Event* event) {
    switch (event->type) {
        // Save data
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
        // Clear much memory
        case SDL_EVENT_LOW_MEMORY:
        // Restore
        case SDL_EVENT_DID_ENTER_FOREGROUND:
            return 0;
    }

    return 1;
}
}

void CLauncher::Create() {
    m_sdlContext = SDL::CContext { SDL_INIT_VIDEO | SDL_INIT_AUDIO };
    SDL_SetEventFilter(HandleAppEvents, nullptr);

    m_window = SDL::CWindow { "Skylabs", 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN };

#ifdef PLATFORM_ANDROID
    SDL_SetWindowFullscreen(*m_window, true);
#endif

    SDL_SetWindowRelativeMouseMode(*m_window, true);

    if (!MIX_Init()) {
        throw std::runtime_error("Cannot initialize SDL_mixer");
    }

    m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!m_mixer) {
        throw std::runtime_error("Cannot create mixer");
    }

    m_track = MIX_CreateTrack(m_mixer);

    std::unique_ptr<IFileStream> stream = Filesystem::LoadAsIO("assets://Interlude.mp3");
    SDL_IOStream* sdlStream = SDL::CreateIOStreamFromResource(stream.get());
    m_music = MIX_LoadAudio_IO(m_mixer, sdlStream, false, false);
    if (!m_music) {
        throw std::runtime_error(fmt::format("Cannot load music: {}", SDL_GetError()));
    }

    MIX_SetTrackAudio(m_track, m_music);

    m_renderer = Vulkan::CRenderer::TryToCreate(&m_window);
    if (!m_renderer) { throw std::runtime_error("Cannot initialize vulkan!\n"); }
}

void CLauncher::Main() {
    float lastFrame = SDL_GetTicks() / 1000.0f;
    int frameCount = 0;
    float elapsedTime = 0.0f;

    MIX_SetTrackGain(m_track, 0.05f);
    MIX_PlayTrack(m_track, 0);

    while (!m_quit) {
        ProcessEvents();

        float currentFrame = SDL_GetTicks() / 1000.0f;
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (!m_minimized) {
            Update(deltaTime);
            Render(deltaTime);
        }

        frameCount++;
        elapsedTime += deltaTime;
        if (elapsedTime >= 1.0f) {
            std::string title = "Skylabs | FPS: " + std::to_string(frameCount);
            SDL_SetWindowTitle(*m_window, title.c_str());
            Log::Debug("{}", title);
            frameCount = 0;
            elapsedTime -= 1.0f;
        }
    }
}

void CLauncher::Destroy() {
    MIX_StopTrack(m_track, MIX_TrackMSToFrames(m_track, 100));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    MIX_DestroyAudio(m_music);
    MIX_DestroyTrack(m_track);
    MIX_DestroyMixer(m_mixer);
    MIX_Quit();
}

void CLauncher::Update(float deltaTime) {
    const std::span keyboardState = SDL::GetKeyboardState();

    if (m_leftJoystick.active) {
        if (std::abs(m_leftJoystick.dirY) > 0.1f) {
            auto direction = (m_leftJoystick.dirY < 0) ? CCamera::MoveDirection::eForward : CCamera::MoveDirection::eBackward;
            m_camera.ProcessKeyboard(direction, deltaTime * std::abs(m_leftJoystick.dirY));
        }

        if (std::abs(m_leftJoystick.dirX) > 0.1f) {
            auto direction = (m_leftJoystick.dirX < 0) ? CCamera::MoveDirection::eLeft : CCamera::MoveDirection::eRight;
            m_camera.ProcessKeyboard(direction, deltaTime * std::abs(m_leftJoystick.dirX));
        }
    } else {
        if (keyboardState[SDL_SCANCODE_W]) m_camera.ProcessKeyboard(CCamera::MoveDirection::eForward, deltaTime);
        if (keyboardState[SDL_SCANCODE_S]) m_camera.ProcessKeyboard(CCamera::MoveDirection::eBackward, deltaTime);
        if (keyboardState[SDL_SCANCODE_A]) m_camera.ProcessKeyboard(CCamera::MoveDirection::eLeft, deltaTime);
        if (keyboardState[SDL_SCANCODE_D]) m_camera.ProcessKeyboard(CCamera::MoveDirection::eRight, deltaTime);
    }

    Uint32 flags = SDL_GetWindowFlags(*m_window);
    m_minimized = flags & SDL_WINDOW_MINIMIZED;
}

void CLauncher::Render(float deltaTime) {
    m_renderer->Draw(m_camera.ViewMatrix(), m_camera.Fov(), deltaTime);
}

void CLauncher::ProcessEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_QUIT:
                m_quit = true;
                break;

            case SDL_EVENT_RENDER_DEVICE_RESET:
                m_renderer->m_needSurfaceRecreation = true;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                m_renderer->m_needSwapchainRecreation = true;
                break;

            case SDL_EVENT_FINGER_DOWN:
            case SDL_EVENT_FINGER_UP:
            case SDL_EVENT_FINGER_MOTION:
                HandleTouchEvent(e);
                break;

            case SDL_EVENT_KEY_DOWN:
                HandleKeyDownEvent(e.key);
                break;

            case SDL_EVENT_KEY_UP:
                HandleKeyUpEvent(e.key);
                break;

            case SDL_EVENT_MOUSE_MOTION:
                m_camera.ProcessMouseMovement(e.motion.xrel, -e.motion.yrel);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                m_camera.ProcessMouseScroll(e.wheel.y);
                break;

            case SDL_EVENT_TEXT_INPUT:
                HandleTextInput(e.text);
                break;
        }
    }
}

void CLauncher::HandleTouchEvent(const SDL_Event& e) {
    if (e.type == SDL_EVENT_FINGER_DOWN) {
        // Open keyboard
        if (m_chatButton.IsInside(e.tfinger.x, e.tfinger.y)) {
            m_textInputActive = !m_textInputActive;
            m_textInputActive ? SDL_StartTextInput(*m_window) : SDL_StopTextInput(*m_window);
            return;
        }

        // Activate joystick
        if (e.tfinger.x < 0.5f && !m_leftJoystick.active) {
            m_leftJoystick.active = true;
            m_leftJoystick.fingerId = e.tfinger.fingerID;
            m_leftJoystick.centerX = e.tfinger.x;
            m_leftJoystick.centerY = e.tfinger.y;
            return;
        }
    }

    if (e.type == SDL_EVENT_FINGER_MOTION) {
        // Joystick movement
        if (m_leftJoystick.active && e.tfinger.fingerID == m_leftJoystick.fingerId) {
            float dx = e.tfinger.x - m_leftJoystick.centerX;
            float dy = e.tfinger.y - m_leftJoystick.centerY;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 0.001f) {
                float scale = (dist > m_leftJoystick.radius) ? (m_leftJoystick.radius / dist) : 1.0f;
                m_leftJoystick.dirX = (dx * scale) / m_leftJoystick.radius;
                m_leftJoystick.dirY = (dy * scale) / m_leftJoystick.radius;
            }

            return;
        }

        // If not joystick finger, then its camera movement
        const auto [width, height] = m_window.DrawableSize();
        m_camera.ProcessMouseMovement(e.tfinger.dx * width, -e.tfinger.dy * height);
    }

    if (e.type == SDL_EVENT_FINGER_UP) {
        // Disable joystick
        if (m_leftJoystick.active && e.tfinger.fingerID == m_leftJoystick.fingerId) {
            m_leftJoystick.active = false;
            m_leftJoystick.dirX = 0.0f;
            m_leftJoystick.dirY = 0.0f;
        }
    }
}

void CLauncher::HandleKeyDownEvent(const SDL_KeyboardEvent& keyEvent) {
    switch (keyEvent.key) {
        case SDLK_ESCAPE: {
            m_quit = true;
        } break;

        case SDLK_RETURN: {
            if (!m_textInputActive)
                break;
            m_textInputActive = false;
            SDL_StopTextInput(*m_window);
            Log::Debug("Keyboard Closed. Final text: {}", m_inputBuffer);
            m_inputBuffer.clear();
        } break;

        case SDLK_P: {
            static bool mouseModeSwitch = false;
            SDL_SetWindowRelativeMouseMode(*m_window, mouseModeSwitch);
            mouseModeSwitch = !mouseModeSwitch;
        } break;

        case SDLK_F11: {
            static bool fullscreenSwitch = true;
            SDL_SetWindowFullscreen(*m_window, fullscreenSwitch);
            fullscreenSwitch = !fullscreenSwitch;
        } break;

        case SDLK_LSHIFT: {
            m_camera.MoveFaster();
        } break;
    }
}

void CLauncher::HandleKeyUpEvent(const SDL_KeyboardEvent& keyEvent) {
    switch (keyEvent.key) {
        case SDLK_LSHIFT: {
            m_camera.ResetSpeed();
        } break;
    }
}

void CLauncher::HandleTextInput(const SDL_TextInputEvent& textEvent) {
    m_inputBuffer += textEvent.text;
    Log::Info("[Input] Current buffer: {}", m_inputBuffer);
}
