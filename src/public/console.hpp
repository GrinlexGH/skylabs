#pragma once
#include "publicapi.hpp"
#include "stc.hpp"

#include <iostream>
#include <array>
#include <format>

template <typename T>
concept Printable = requires(std::ostream& os, T a) { os << a; };

class CDefaultConsoleMessage {
public:
    template <typename... Args>
    void operator()(const std::format_string<Args...> fmt, Args&&... args) {
        std::cout
            << stc::reset_fg
            << std::format(fmt, std::forward<Args>(args)...);
    }

private:
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

class CColorfulConsoleMessage {
public:
    using RGB_t = std::array<std::uint8_t, 3>;
    explicit CColorfulConsoleMessage(const RGB_t& color) : m_color(color) { }

    template <typename... Args>
    void operator()(const std::format_string<Args...> fmt, Args&&... args) {
        std::cout
            << stc::rgb_fg(m_color[0], m_color[1], m_color[2])
            << std::format(fmt, std::forward<Args>(args)...)
            << stc::reset_fg;
    }

private:
    RGB_t m_color { 255, 255, 255 };

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

PLATFORM_GLOBAL CDefaultConsoleMessage Msg;
PLATFORM_GLOBAL CColorfulConsoleMessage Warning;
PLATFORM_GLOBAL CColorfulConsoleMessage Error;
