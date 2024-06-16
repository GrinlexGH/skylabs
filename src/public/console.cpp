#include "console.hpp"
#include <cstdarg>
#include <cstdio>
#include <string_view>

void console::Msg(std::string_view format, ...) {
  std::va_list argList;

  va_start(argList, format);
  vprintf(format.data(), argList);
  va_end(argList);
}