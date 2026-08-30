#pragma once
#include <skylabs/public/pch.hpp>

enum class Keys : char
{
    eUnknown = 0,
    eEscape,
    eLeftShift,
    eZ,
    eEnter,
    eF11,
};

struct UnknownEvent { };
struct QuitEvent { };
struct DeviceResetEvent { }; // Important on Android, when user reopen an app
struct WindowResizeEvent { int width, height; };
struct KeyEvent { Keys key; bool down; };
struct MouseMotionEvent { float dx, dy; };
struct MouseWheelEvent { float y; };
struct MouseButtonEvent { bool down; };
struct TextInputEvent { std::string text; };
struct FingerTouchEvent { bool down; float x, y; unsigned int fingerID; };
struct FingerMotionEvent { float x, y, dx, dy; unsigned int fingerID; };

using Event = std::variant<
    UnknownEvent,
    QuitEvent,
    DeviceResetEvent,
    WindowResizeEvent,
    KeyEvent,
    MouseMotionEvent,
    MouseWheelEvent,
    MouseButtonEvent,
    TextInputEvent,
    FingerTouchEvent,
    FingerMotionEvent
>;

using EventFilter = bool(*)(const Event& event, void* userData);

class PUBLIC_CLASS IEventPump
{
public:
    virtual ~IEventPump() = default;

    virtual void SetEventFilter(EventFilter filter, void* userData) = 0;
    [[nodiscard]] virtual std::optional<Event> PollEvent() = 0;
};
