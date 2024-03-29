#include <string_view>
#include <cstdarg>
#include "console.hpp"

void console::Msg(std::string_view format, ...) {
    std::va_list argList;

    va_start(argList, format);
    vprintf(format.data(), argList);
    va_end(argList);
}