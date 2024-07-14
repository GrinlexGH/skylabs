#include "console.hpp"

#include "platform.hpp"
#include "stc.hpp"

#include <cstdarg>
#include <cstdio>
#include <functional>
#include <iostream>
#include <string_view>

void CConsoleMessage::operator()(std::string_view format, ...) {
    std::va_list argList;
    va_start(argList, format);
    vprintf(format.data(), argList);
    va_end(argList);
}

void CConsoleMessage::_AcceptOstreamManips(std::ostream &(*f)(std::ostream &)) {
    f(std::cout);
}

void CConsoleMessage::_AcceptIosManips(std::ostream &(*f)(std::ios &)) {
    f(std::cout);
}

void CConsoleMessage::_AcceptIosBaseManips(
    std::ostream &(*f)(std::ios_base &)) {
    f(std::cout);
}

PLATFORM_CLASS CConsoleMessage Msg;
PLATFORM_CLASS CConsoleMessage Warning{CConsoleMessage::rgb{215, 135, 0}};
PLATFORM_CLASS CConsoleMessage Error{CConsoleMessage::rgb{175, 0, 0}};
