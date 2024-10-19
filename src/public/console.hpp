#pragma once
#include "platform.hpp"
#include "stc.hpp"

#include <iostream>
#include <string_view>
#include <array>
#include <cstdint>

template <typename T>
concept Printable = requires(std::ostream& os, T a) { os << a; };

class IConsoleMessage {
public:
    virtual ~IConsoleMessage() = default;

    // Printing to console in same format as printf()
    PLATFORM_CLASS virtual void operator()(std::string_view format, ...) = 0;
};

class CDefaultConsoleMessage final : IConsoleMessage {
public:
    PLATFORM_CLASS virtual void operator()(std::string_view format, ...);

protected:
    PLATFORM_CLASS friend CDefaultConsoleMessage&
    operator<<(CDefaultConsoleMessage& s, std::ostream& (*f)(std::ostream&));

    PLATFORM_CLASS friend CDefaultConsoleMessage&
    operator<<(CDefaultConsoleMessage& s, std::ostream& (*f)(std::ios&));

    PLATFORM_CLASS friend CDefaultConsoleMessage&
    operator<<(CDefaultConsoleMessage& s, std::ostream& (*f)(std::ios_base&));

    template <Printable T>
    friend CDefaultConsoleMessage& operator<<(CDefaultConsoleMessage& s, const T& message) {
        std::cout << stc::reset_fg << message;
        return s;
    }
};

class CColorfulConsoleMessage final : public IConsoleMessage {
public:
    using rgb_t = std::array<std::uint8_t, 3>;

    CColorfulConsoleMessage() = default;
    CColorfulConsoleMessage(rgb_t color) : m_color(color) { }
    CColorfulConsoleMessage(const CColorfulConsoleMessage&) = default;
    CColorfulConsoleMessage(CColorfulConsoleMessage&&) = default;
    CColorfulConsoleMessage& operator=(const CColorfulConsoleMessage&) = default;
    CColorfulConsoleMessage& operator=(CColorfulConsoleMessage&&) = default;
    virtual ~CColorfulConsoleMessage() = default;

    PLATFORM_CLASS virtual void operator()(std::string_view format, ...);

protected:
    rgb_t m_color { 255, 255, 255 };

protected:
    PLATFORM_CLASS friend CColorfulConsoleMessage&
    operator<<(CColorfulConsoleMessage& s, std::ostream& (*f)(std::ostream&));

    PLATFORM_CLASS friend CColorfulConsoleMessage&
    operator<<(CColorfulConsoleMessage& s, std::ostream& (*f)(std::ios&));

    PLATFORM_CLASS friend CColorfulConsoleMessage&
    operator<<(CColorfulConsoleMessage& s, std::ostream& (*f)(std::ios_base&));

    template <Printable T>
    friend CColorfulConsoleMessage& operator<<(CColorfulConsoleMessage& s, const T& message) {
        std::cout << stc::rgb_fg(s.m_color[0], s.m_color[1], s.m_color[2]) << message << stc::reset_fg;
        return s;
    }
};

PLATFORM_OVERLOAD CDefaultConsoleMessage Msg;
PLATFORM_OVERLOAD CColorfulConsoleMessage Warning;
PLATFORM_OVERLOAD CColorfulConsoleMessage Error;
