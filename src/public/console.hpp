#pragma once
#include "platform.hpp"
#include "stc.hpp"

#include <iostream>
#include <string_view>

template <typename T>
concept Printable = requires(std::ostream& os, T a) { os << a; };

class IConsoleMessage {
public:
    // Printing to console in same format as printf()
    PLATFORM_CLASS virtual void operator()(std::string_view format, ...)    = 0;

protected:
    virtual void _AcceptOstreamManips(std::ostream& (*f)(std::ostream&))    = 0;
    virtual void _AcceptIosManips    (std::ostream& (*f)(std::ios&))        = 0;
    virtual void _AcceptIosBaseManips(std::ostream& (*f)(std::ios_base&))   = 0;
};

class CConsoleMessage : public IConsoleMessage {
public:
    struct rgb { int r, g, b; };

    CConsoleMessage()           = default;
    CConsoleMessage(rgb col) : Color_(col) { }
    virtual ~CConsoleMessage()  = default;
    PLATFORM_CLASS virtual void operator()(std::string_view format, ...);

protected:
    rgb Color_ { 255, 255, 255 };

protected:
    virtual void _AcceptOstreamManips(std::ostream& (*f)(std::ostream&));
    virtual void _AcceptIosManips    (std::ostream& (*f)(std::ios&));
    virtual void _AcceptIosBaseManips(std::ostream& (*f)(std::ios_base&));

    PLATFORM_CLASS friend CConsoleMessage&
    operator<<(CConsoleMessage& s, std::ostream& (*f)(std::ostream&));

    PLATFORM_CLASS friend CConsoleMessage&
    operator<<(CConsoleMessage& s, std::ostream& (*f)(std::ios&));

    PLATFORM_CLASS friend CConsoleMessage&
    operator<<(CConsoleMessage& s, std::ostream& (*f)(std::ios_base&));

    template <Printable T>
    friend CConsoleMessage& operator<<(CConsoleMessage& s, const T& message) {
        std::cout << stc::rgb_fg(s.Color_.r, s.Color_.g, s.Color_.b) << message
                  << stc::reset;
        return s;
    }
};

PLATFORM_OVERLOAD CConsoleMessage Msg;
PLATFORM_OVERLOAD CConsoleMessage Warning;
PLATFORM_OVERLOAD CConsoleMessage Error;
