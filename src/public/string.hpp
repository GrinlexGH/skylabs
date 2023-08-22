#pragma once

#include <string>
#include <stdexcept>
#include <cstdint>
#include <iostream>

class String
{
protected:
    std::u8string str;

    std::wstring ConvertUTF8ToWideString(const std::u8string& in) const;
    std::u8string ConvertWideStringToUTF8(const std::wstring& in) const;

public:
    String() = default;
    String(const char8_t* str) : str(str)
     { }
    String(const std::u8string& str) : str(str)
     { }
    String(const wchar_t* str) : str(ConvertWideStringToUTF8(str))
     { }


    operator const wchar_t*() const {
        return ConvertUTF8ToWideString(str).c_str();
    }

    String& operator=(const wchar_t* wideStr)
    {
        str = this->ConvertWideStringToUTF8(std::wstring(wideStr));
        return *this;
    }
};

std::u8string String::ConvertWideStringToUTF8(const std::wstring& in) const {
    std::u8string out;
    // Заранее резервируем память для строки "out", уменьшая число перераспределений
    out.reserve(in.size() * 4);
    std::uint32_t codePoint = 0;
    for (wchar_t wchar : in) {
        /*
        * +--------------+--------------+----------+----------+----------+----------+
        * | Code point   | Code point   | Byte 1   | Byte 2   | Byte 3   | Byte 4   |
        * | (First)      | (Last)       |          |          |          |          |
        * +--------------+--------------+----------+----------+----------+----------+
        * | U+0000       | U+007F       | 0xxxxxxx |          |          |          |
        * | U+0080       | U+07FF       | 110xxxxx | 10xxxxxx |          |          |
        * | U+0800       | U+FFFF       | 1110xxxx | 10xxxxxx | 10xxxxxx |          |
        * | U+10000      | U+10FFFF     | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |
        * +--------------+--------------+----------+----------+----------+----------+
        */
        // Однобайтовый символ
        if (wchar <= 0x7F) {
            out.push_back(static_cast<char8_t>(wchar));
        }
        // Двухбайтовый символ
        else if (wchar <= 0x07FF) {
            /*
            * Например, у нас есть символ 0000001110100110 (0x03A6) и нужно его
            * перевести в 11001110 10100110 (0xCE 0xA6)
            * Первый байт:
            * 1. Смещаем вправо на 6: 00001110
            * 2. Прибавляем 11000000: 11001110 -- получили старший байт
            * Второй байт:
            * 1. К 10100110 (первые 8 бит нашего символа 0x03A6) применяем
            *       маску 00111111 (0x3F): 00100110
            * 2. Складываем с 10000000 (0x80): 10100110
            */
            out.push_back(static_cast<char8_t>((wchar >> 6) | 0xC0));
            out.push_back(static_cast<char8_t>((wchar & 0x3F) | 0x80));
        }
        // На винде wchar_t равен 16 бит, а на линуксе 32
#ifdef _WIN32
        /*
        * Всё по формуле из https://en.wikipedia.org/wiki/UTF-16#Examples
        * Например, у нас есть 0xD801 0xDC37
        * Для старшего суррогата:
        * 1. Из старшего суррогата 0xD801 вычитаем 0xD800: (0x0001)
        * 2. Умножаем на 0x400: 0x0001 * 0x400 == 0x0400
        * Для младшего суррогата:
        * 1. Из младшего суррогата 0xDC37 вычитаем 0xDC00: 00110111 (0x37)
        * Финал:
        * 1. Складываем полученные результаты: 0x0400 + 0x37 == 0x0437
        * 2. Прибавляем 0x10000: 0x0437 + 0x10437
        * И вуаля! У нас есть utf32 символ, который уже просто перевести в utf8 (см. ConvertUTF8ToWideString)!
        */
        // Суррогаты четырёхбайтовых символов
        // Старший суррогат: U+D800 - U+DBFF
        // Младший суррогат: U+DC00 - U+DFFF
        else if (wchar >= 0xD800 && wchar <= 0xDBFF) {
            codePoint = ((wchar - 0xD800) * 0x400);
        }
        else if (wchar >= 0xDC00 && wchar <= 0xDFFF) {
            if (codePoint == 0) {
                throw std::invalid_argument("Invalid UTF-16 sequence: unexpected low surrogate");
            }
            codePoint += (wchar - 0xDC00);
            codePoint += 0x10000;
            if (codePoint > 0x10FFFF) {
                throw std::invalid_argument("Invalid UTF-16 sequence: code point out of range");
            }
            out.push_back(static_cast<char8_t>((codePoint >> 18) | 0xF0));
            out.push_back(static_cast<char8_t>(((codePoint >> 12) & 0x3F) | 0x80));
            out.push_back(static_cast<char8_t>(((codePoint >> 6) & 0x3F) | 0x80));
            out.push_back(static_cast<char8_t>((codePoint & 0x3F) | 0x80));
        }
        /*
        * Абсолютно тоже самое, что и в двухбайтовых символах,
        * только с учётом ещё одного байта
        */
        // Трёхбайтовые символы
        else {
            out.push_back(static_cast<char8_t>((wchar >> 12) | 0xE0));
            out.push_back(static_cast<char8_t>(((wchar >> 6) & 0x3F) | 0x80));
            out.push_back(static_cast<char8_t>((wchar & 0x3F) | 0x80));
        }
        // На винде wchar_t равен 16 бит, а на линуксе 32
#else
        // Четырёхбайтовые символы
        else {
            out.push_back(static_cast<char8_t>((wchar >> 18) | 0xF0));
            out.push_back(static_cast<char8_t>(((wchar >> 12) & 0x3F) | 0x80));
            out.push_back(static_cast<char8_t>(((wchar >> 6) & 0x3F) | 0x80));
            out.push_back(static_cast<char8_t>((wchar & 0x3F) | 0x80));
        }
#endif
    }
    return out;
}

std::wstring String::ConvertUTF8ToWideString(const std::u8string& in) const {
    std::wstring out;
    // Заранее резервируем память для строки "out", уменьшая число перераспределений
    // 1 utf8 byte  (in.size() / 2 == 0 (искл.)) -- 1 utf16 byte
    // 2 utf8 bytes (in.size() / 2 == 1)         -- 1 utf16 byte
    // 3 utf8 bytes (in.size() / 2 == 1{.5})     -- 1 utf16 byte
    // 4 utf8 bytes (in.size() / 2 == 2)         -- 2 utf16 bytes
    out.reserve(in.size() > 1 ? in.size() / 2 : 1);
    for (size_t i = 0; i < in.size(); ) {
        // Если первый бит равен нулю
        // то это однобайтовый символ
        if ((in[i] & 0x80) == 0) {
            out.push_back(static_cast<wchar_t>(in[i]));
            i += 1;
        }
        // Если первые три бита равны 110
        // то это двухбайтовый символ
        else if ((in[i] & 0xE0) == 0xC0) {
            /*
            * Пример: у нас есть 2 байта -- 11010001(in[i]) и 10001000(in[i+1])
            * Применяем к первому байту маску 00011111 и тем самым заменяем
            * старшие три бита нулями и получаем 00010001(in[i])
            *
            * Применяем маску 00111111 ко второму байту и тем самым заполняем
            * старшие два бита нулями и получаем 00001000(in[i+1])
            *
            * Сдвигаем на 6 битов влево чтобы освободить место под те биты,
            * которые нам нужны из второго байта in[i+1] и получаем
            * 00000000 00000000 00000100 01000000 (это уже внутри in)
            *
            * Просто добавляем к этой переменной ещё биты из второго байта in[i+1]
            * и получаем 00000000 00000000 00000100 01001000
            * И вуаля! у нас есть UTF32 символ!
            */
            out.push_back(static_cast<wchar_t>(((in[i] & 0x1F) << 6) | (in[i + 1] & 0x3F)));
            i += 2;
        }
        // Если первые четыре бита равны 1110
        // то это трёхбайтовый символ
        else if ((in[i] & 0xF0) == 0xE0) {
            /*
            * Всё тоже самое что и выше, только маска для высшего байта заменяет уже высшие
            * 4 бита на ноль, и всё сдвигается с учётом ещё одного байта
            */
            out.push_back(static_cast<wchar_t>(((in[i] & 0x0F) << 12) | ((in[i + 1] & 0x3F) << 6) | (in[i + 2] & 0x3F)));
            i += 3;
        }
        // Если первые пять битов равны 11110
        // то это четырёхбайтовый символ
        else if ((in[i] & 0xF8) == 0xF0) {
            // На винде wchar_t равен 16 бит, а на линуксе 32
    #if defined(_WIN32) && !(defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
            /*
            * Всё тоже самое что и выше, только маска для высшего байта заменяет уже высшие
            * 5 бита на ноль, и всё сдвигается с учётом ещё одного байта
            */
            uint32_t u32 = ((in[i] & 0x07) << 24) | ((in[i + 1] & 0x3F) << 12) | ((in[i + 2] & 0x3F) << 6) | (in[i + 3] & 0x3F);
            /*
            * Всё делается исключительно по формуле от сюда:
            * https://en.wikipedia.org/wiki/UTF-16#Examples
            * Пример: У нас есть 0x00010437 в UTF32
            * Сначала вычитаем 0x10000 и получаем 0x00000437
            * Сдвигаем на 10 вправо и прибавляем 0xD800 чтобы найти высший суррогат
            * Сдвигаем высший суррогат на 16 влево чтобы освободить место под младший суррогат
            * Берём младшие 10 битов (остаток от деления на 0x400) и прибавляем 0xDC00
            * Просто собираем эти два байта вместе
            * И вуаля! суррогатная пара UTF16!
            */
            out.push_back(static_cast<wchar_t>(((u32 - 0x10000) >> 10) + 0xD800));
            out.push_back(static_cast<wchar_t>((u32 % 0x400) + 0xDC00));
    #else
            out.push_back(static_cast<wchar_t>(((in[i] & 0x07) << 24) | ((in[i + 1] & 0x3F) << 12) | ((in[i + 2] & 0x3F) << 6) | (in[i + 3] & 0x3F)));
    #endif
            i += 4;
        }
        // А это уже не UTF8 символ
        else {
            throw std::invalid_argument("Invalid character");
            break;
        }
    }
    return out;
}