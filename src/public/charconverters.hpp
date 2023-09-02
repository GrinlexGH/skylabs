#pragma once

#include <string>

#define WideCharIsUTF16

namespace CharConverters
{
    /**
    * @brief ѕринимает как шаблон тип возвращаемого значени€,
    */
    template<typename outStringT>
    outStringT WideStrToUTF8(const std::wstring_view in);
    template<typename inStringT>
    std::wstring UTF8ToWideStr(const inStringT in);
}

