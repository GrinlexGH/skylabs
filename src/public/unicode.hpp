#ifdef WIN32
#pragma once

#include <stdexcept>

extern inline std::wstring Utf8ToUtf16(std::string_view utf8);
extern inline std::string Utf16ToUtf8(std::wstring_view utf16);

#endif

