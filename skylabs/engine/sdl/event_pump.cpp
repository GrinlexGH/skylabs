#include <frozen/unordered_map.h>

#include "skylabs/engine/sdl/event_pump.hpp"

namespace {
constexpr frozen::unordered_map<SDL_Keycode, sk::input::Keys, 6> kKeyMap {
    { SDLK_UNKNOWN, sk::input::Keys::eUnknown },  { SDLK_ESCAPE, sk::input::Keys::eEscape },
    { SDLK_LSHIFT, sk::input::Keys::eLeftShift }, { SDLK_Z, sk::input::Keys::eZ },
    { SDLK_RETURN, sk::input::Keys::eEnter },     { SDLK_F11, sk::input::Keys::eF11 },
};

sk::input::Event TranslateEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_QUIT:
            return sk::input::QuitEvent { };
        case SDL_EVENT_RENDER_DEVICE_RESET:
            return sk::input::DeviceResetEvent { };
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            return sk::input::WindowResizeEvent { .width = event.window.data1,
                                                  .height = event.window.data2 };
        case SDL_EVENT_WINDOW_EXPOSED:
            return sk::input::WindowExposeEvent { };
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if (!kKeyMap.contains(event.key.key)) return sk::input::UnknownEvent { };
            return sk::input::KeyEvent { .key = kKeyMap.at(event.key.key),
                                         .down = event.type == SDL_EVENT_KEY_DOWN };
        case SDL_EVENT_MOUSE_MOTION:
            return sk::input::MouseMotionEvent { .dx = event.motion.xrel, .dy = event.motion.yrel };
        case SDL_EVENT_MOUSE_WHEEL:
            return sk::input::MouseWheelEvent { event.wheel.y };
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            return sk::input::MouseButtonEvent { event.type == SDL_EVENT_MOUSE_BUTTON_DOWN };
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_UP:
            return sk::input::FingerTouchEvent { .down = event.type == SDL_EVENT_FINGER_DOWN,
                                                 .x = event.tfinger.x,
                                                 .y = event.tfinger.y,
                                                 .fingerID =
                                                     static_cast<unsigned int>(event.tfinger.fingerID) };
        case SDL_EVENT_FINGER_MOTION:
            return sk::input::FingerMotionEvent { .x = event.tfinger.x,
                                                  .y = event.tfinger.y,
                                                  .dx = event.tfinger.dx,
                                                  .dy = event.tfinger.dy,
                                                  .fingerID = static_cast<unsigned int>(
                                                      event.tfinger.fingerID) };
        default:
            return sk::input::UnknownEvent { };
    }
}
}

namespace sk::sdl {
void EventPump::SetEventFilter(const input::EventFilter filter, void* userData) {
    m_filterUserData = userData;
    m_filter = filter;
    SDL_SetEventFilter(EventFilterWrap, this);
}

std::optional<input::Event> EventPump::PollEvent() {
    SDL_Event event;
    if (!SDL_PollEvent(&event)) {
        return std::nullopt;
    }

    return TranslateEvent(event);
}

bool EventPump::EventFilterWrap(void* userData, SDL_Event* const event) {
    const auto self = static_cast<EventPump*>(userData);
    return self->m_filter(TranslateEvent(*event), self->m_filterUserData);
}
}
