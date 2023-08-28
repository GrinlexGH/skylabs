#pragma once

#include <string>

#define WideCharIsUTF16

namespace CharConverters
{
    std::string WideStrToUTF8(const std::wstring_view in);
    std::wstring UTF8ToWideStr(const std::string_view in);
}

