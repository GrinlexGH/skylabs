#pragma once
#include <iostream>
#include <string_view>

#include "platform.hpp"
#include "stc.hpp"

class IConsoleMessage {
public:
    struct rgb {
        int r, g, b;
    };
    IConsoleMessage() = default;
    IConsoleMessage(rgb col) : Color(col) {}
    virtual ~IConsoleMessage() = default;
    PLATFORM_CLASS virtual void operator()(std::string_view format, ...);
    rgb Color{ 255, 255, 255 };

protected:
    virtual void _AcceptOstreamManips(std::ostream &(*f)(std::ostream &));
    virtual void _AcceptIosManips(std::ostream &(*f)(std::ios &));
    virtual void _AcceptIosBaseManips(std::ostream &(*f)(std::ios_base &));
    PLATFORM_CLASS friend IConsoleMessage &
    operator<<(IConsoleMessage &s, std::ostream &(*f)(std::ostream &));
    PLATFORM_CLASS friend IConsoleMessage &
    operator<<(IConsoleMessage &s, std::ostream &(*f)(std::ios &));
    PLATFORM_CLASS friend IConsoleMessage &
    operator<<(IConsoleMessage &s, std::ostream &(*f)(std::ios_base &));
};

template <typename T>
IConsoleMessage &operator<<(IConsoleMessage &s, const T &x) {
    std::cout << x;
    return s;
}

struct CConsoleInfoMsg : public IConsoleMessage {
    using IConsoleMessage::IConsoleMessage; // Using constructors of base class
    PLATFORM_CLASS void operator()(std::string_view format, ...) override;
};

template <typename T>
CConsoleInfoMsg &operator<<(CConsoleInfoMsg &s, const T &x) {
    std::cout << stc::rgb_fg(s.Color.r, s.Color.g, s.Color.b) << x << stc::reset;
     return s;
}

struct CConsoleErrorMsg : public IConsoleMessage {
    using IConsoleMessage::IConsoleMessage; // Using constructors of base class
    PLATFORM_CLASS void operator()(std::string_view format, ...) override;
};

template <typename T>
CConsoleErrorMsg &operator<<(CConsoleErrorMsg &s, const T &x) {
    std::cout << stc::rgb_fg(s.Color.r, s.Color.g, s.Color.b) << x << stc::reset;
    return s;
}

extern PLATFORM_CLASS CConsoleInfoMsg Msg;
extern PLATFORM_CLASS CConsoleErrorMsg Error;
