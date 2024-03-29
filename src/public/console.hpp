#ifndef CONSOLE_HPP
#define CONSOLE_HPP
#ifdef _WIN32
#pragma once
#endif
#include <string_view>

namespace console {
    void Msg(std::string_view format, ...);
}

#endif