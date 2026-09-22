#pragma once
#include <optional>
#include <string>
#include <variant>

#include "sk_base_export.h"

namespace sk::input {
enum class Keys : std::uint8_t {
    eUnknown = 0,
    eEscape,
    eLeftShift,
    eZ,
    eEnter,
    eF11,
};

struct UnknownEvent { };
struct QuitEvent { };
struct DeviceResetEvent { };  // Important on Android, when user reopen an app
struct WindowResizeEvent {
    int width, height;
};

struct WindowExposeEvent { };
struct KeyEvent {
    Keys key;
    bool down;
};

struct MouseMotionEvent {
    float dx, dy;
};

struct MouseWheelEvent {
    float y;
};

struct MouseButtonEvent {
    bool down;
};

struct TextInputEvent {
    std::string text;
};

struct FingerTouchEvent {
    bool down;
    float x, y;
    unsigned int fingerID;
};

struct FingerMotionEvent {
    float x, y, dx, dy;
    unsigned int fingerID;
};

using Event = std::variant<UnknownEvent, QuitEvent, DeviceResetEvent, WindowResizeEvent,
                           WindowExposeEvent, KeyEvent, MouseMotionEvent, MouseWheelEvent,
                           MouseButtonEvent, TextInputEvent, FingerTouchEvent, FingerMotionEvent>;

using EventFilter = bool (*)(const Event& event, void* userData);

class SK_BASE_PUBLIC_CLASS IEventPump {
public:
    virtual ~IEventPump() = default;

    virtual void SetEventFilter(EventFilter filter, void* userData) = 0;
    [[nodiscard]] virtual std::optional<Event> PollEvent() = 0;
};
}
