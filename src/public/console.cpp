#include "console.hpp"

#include "platform.hpp"
#include "stc.hpp"

#include <cstdarg>

void CDefaultConsoleMessage::operator()(std::string_view format, ...) {
    std::cout << stc::reset_fg;
    std::va_list argList;
    va_start(argList, format);
    vprintf(format.data(), argList);
    va_end(argList);
}

CDefaultConsoleMessage& operator<<(CDefaultConsoleMessage& s, std::ostream& (*f)(std::ostream&)) {
    f(std::cout);
    return s;
}

CDefaultConsoleMessage& operator<<(CDefaultConsoleMessage& s, std::ostream& (*f)(std::ios&)) {
    f(std::cout);
    return s;
}

CDefaultConsoleMessage& operator<<(CDefaultConsoleMessage& s, std::ostream& (*f)(std::ios_base&)) {
    f(std::cout);
    return s;
}

void CColorfulConsoleMessage::operator()(std::string_view format, ...) {
    std::cout << stc::rgb_fg(m_color[0], m_color[1], m_color[2]);
    std::va_list argList;
    va_start(argList, format);
    vprintf(format.data(), argList);
    va_end(argList);
    std::cout << stc::reset_fg;
}

CColorfulConsoleMessage& operator<<(CColorfulConsoleMessage& s, std::ostream& (*f)(std::ostream&)) {
    f(std::cout);
    return s;
}

CColorfulConsoleMessage& operator<<(CColorfulConsoleMessage& s, std::ostream& (*f)(std::ios&)) {
    f(std::cout);
    return s;
}

CColorfulConsoleMessage& operator<<(CColorfulConsoleMessage& s, std::ostream& (*f)(std::ios_base&)) {
    f(std::cout);
    return s;
}

PLATFORM_CLASS CDefaultConsoleMessage Msg;
PLATFORM_CLASS CColorfulConsoleMessage Warning { { 215, 135, 0 } };
PLATFORM_CLASS CColorfulConsoleMessage Error { { 175, 0, 0 } };
