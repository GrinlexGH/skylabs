#include "console.hpp"
#include "stc.hpp"
#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <string_view>

void IConsoleMessage::operator()(std::string_view format, ...) {
    std::va_list argList;
    va_start(argList, format);
    vprintf(format.data(), argList);
    va_end(argList);
}

void IConsoleMessage::_AcceptOstreamManips(std::ostream& (*f)(std::ostream &)) {
    f(std::cout);
}

IConsoleMessage& operator<< (IConsoleMessage &s, std::ostream& (*f)(std::ostream &)) {
    s._AcceptOstreamManips(f);
    return s;
}

void IConsoleMessage::_AcceptIosManips(std::ostream& (*f)(std::ios &)) {
    f(std::cout);
}

IConsoleMessage& operator<< (IConsoleMessage &s, std::ostream& (*f)(std::ios &)) {
    s._AcceptIosManips(f);
    return s;
}

void IConsoleMessage::_AcceptIosBaseManips(std::ostream& (*f)(std::ios_base &)) {
    f(std::cout);
}

IConsoleMessage& operator<< (IConsoleMessage &s, std::ostream& (*f)(std::ios_base &)) {
    s._AcceptIosBaseManips(f);
    return s;
}

void CConsoleInfoMsg::operator()(std::string_view format, ...) {
    std::cout << stc::rgb_fg(Color.r, Color.g, Color.b);
    std::va_list args;
    va_start(args, format);
    vprintf(format.data(), args);
    va_end(args);
    std::cout << stc::reset;
}

void CConsoleErrorMsg::operator()(std::string_view format, ...) {
    std::va_list argList;
    std::cout << stc::rgb_fg(Color.r, Color.g, Color.b);
    va_start(argList, format);
    vprintf(format.data(), argList);
    va_end(argList);
    std::cout << stc::reset;
}

CConsoleInfoMsg Msg { IConsoleMessage::rgb {0, 175, 215} };
CConsoleErrorMsg Error { IConsoleMessage::rgb {175, 0, 0} };
