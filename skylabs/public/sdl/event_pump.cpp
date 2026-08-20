#include <skylabs/public/sdl/event_pump.hpp>

namespace {
constexpr frozen::unordered_map<SDL_Keycode, Keys, 6> g_keyMap {
    { SDLK_UNKNOWN, Keys::eUnknown },
    { SDLK_ESCAPE, Keys::eEscape },
    { SDLK_LSHIFT, Keys::eLeftShift },
    { SDLK_Z, Keys::eZ },
    { SDLK_RETURN, Keys::eEnter },
    { SDLK_F11, Keys::eF11 },
};
}

namespace SDL {
std::optional<Event> CEventPump::PollEvent() {
    SDL_Event event;
    if (!SDL_PollEvent(&event)) {
        return std::nullopt;
    }

    switch (event.type) {
        case SDL_EVENT_QUIT:
            return QuitEvent { };
        case SDL_EVENT_RENDER_DEVICE_RESET:
            return DeviceResetEvent { };
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            return WindowResizeEvent { event.window.data1, event.window.data2 };
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if (!g_keyMap.contains(event.key.key)) { return std::nullopt; }
            return KeyEvent { g_keyMap.at(event.key.key), event.type == SDL_EVENT_KEY_DOWN };
        case SDL_EVENT_MOUSE_MOTION:
            return MouseMotionEvent { event.motion.xrel, event.motion.yrel };
        case SDL_EVENT_MOUSE_WHEEL:
            return MouseWheelEvent { event.wheel.y };
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            return MouseButtonEvent { event.type == SDL_EVENT_MOUSE_BUTTON_DOWN };
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_UP:
            return FingerTouchEvent {
                event.type == SDL_EVENT_FINGER_DOWN,
                event.tfinger.x, event.tfinger.y,
                static_cast<unsigned int>(event.tfinger.fingerID)
            };
        case SDL_EVENT_FINGER_MOTION:
            return FingerMotionEvent {
                event.tfinger.x, event.tfinger.y,
                event.tfinger.dx, event.tfinger.dy,
                static_cast<unsigned int>(event.tfinger.fingerID)
            };
        default:
            return UnknownEvent { };
    }
}
}
