#pragma once

#include <string>
#include <cstdint>
#include <stdexcept>

#ifdef _WIN32
#define WideCharIsUTF16
#else
#define WideCharIsUTF32
#endif

namespace CharConverters
{
    template<typename OutStringT>
    OutStringT WideStrToUTF8(const std::wstring_view in) {
        static_assert(
            std::is_same_v<OutStringT, std::string> || std::is_same_v<OutStringT, std::u8string>,
            "Unsupported output type"
            );
        if (in.empty()) {
            return OutStringT();
        }
        OutStringT out;
        out.reserve(in.size() * 4);
        uint32_t codePoint = 0;
        for (wchar_t wchar : in) {
            if (wchar <= 0x7F) {
                out.push_back(static_cast<char8_t>(wchar));
            }
            else if (wchar <= 0x07FF) {
                out.push_back(static_cast<char8_t>((wchar >> 6) | 0xC0));
                out.push_back(static_cast<char8_t>((wchar & 0x3F) | 0x80));
            }
#ifdef WideCharIsUTF16
            else if (wchar >= 0xD800 && wchar <= 0xDBFF) {
                codePoint = ((wchar - 0xD800) * 0x400);
            }
            else if (wchar >= 0xDC00 && wchar <= 0xDFFF) {
                if (codePoint >= 0xD800 && codePoint <= 0xDFFF) {
                    throw std::invalid_argument("Invalid UTF-16 sequence: unexpected low surrogate");
                }
                codePoint += (wchar - 0xDC00);
                codePoint += 0x10000;
                if (codePoint > 0x10FFFF) {
                    throw std::invalid_argument("Invalid UTF-16 sequence: low surrogate is out of range");
                }
                out.push_back(static_cast<char8_t>((codePoint >> 18) | 0xF0));
                out.push_back(static_cast<char8_t>(((codePoint >> 12) & 0x3F) | 0x80));
                out.push_back(static_cast<char8_t>(((codePoint >> 6) & 0x3F) | 0x80));
                out.push_back(static_cast<char8_t>((codePoint & 0x3F) | 0x80));
            }
#endif
            else {
                out.push_back(static_cast<char8_t>((wchar >> 12) | 0xE0));
                out.push_back(static_cast<char8_t>(((wchar >> 6) & 0x3F) | 0x80));
                out.push_back(static_cast<char8_t>((wchar & 0x3F) | 0x80));
            }
#ifdef WideCharIsUTF32
            else if (wchar <= 0x10FFFF) {
                out.push_back(static_cast<char8_t>((wchar >> 18) | 0xF0));
                out.push_back(static_cast<char8_t>(((wchar >> 12) & 0x3F) | 0x80));
                out.push_back(static_cast<char8_t>(((wchar >> 6) & 0x3F) | 0x80));
                out.push_back(static_cast<char8_t>((wchar & 0x3F) | 0x80));
            }
#endif
        }
        return out;
    }

    template<typename InStringT>
    std::wstring UTF8ToWideStr(const InStringT in) {
        static_assert(
            (std::is_same_v<InStringT, std::string_view> || std::is_same_v<InStringT, std::u8string_view>)
            || (std::is_same_v<InStringT, std::string> || std::is_same_v<InStringT, std::u8string>),
            "Unsupported output type"
            );
        if (in.empty()) {
            return std::wstring(L"");
        }
        std::wstring out;
        out.reserve(in.size() > 1 ? in.size() / 2 : 1);
        for (size_t i = 0; i < in.size(); ) {
            if ((in[i] & 0x80) == 0) {
                out.push_back(static_cast<wchar_t>(in[i]));
                i += 1;
            }
            else if ((in[i] & 0xE0) == 0xC0) {
                // Checking for a valid utf8 sequence
                if (i + 1 < in.size() && (in[i + 1] & 0xC0) == 0x80) {
                    out.push_back(static_cast<wchar_t>(((in[i] & 0x1F) << 6) | (in[i + 1] & 0x3F)));
                    i += 2;
                }
                else {
                    throw std::invalid_argument("Invalid UTF-8 sequence");
                }
            }
            else if ((in[i] & 0xF0) == 0xE0) {
                // Checking for a valid utf8 sequence
                if (i + 2 < in.size() && (in[i + 1] & 0xC0) == 0x80 && (in[i + 2] & 0xC0) == 0x80) {
                    out.push_back(static_cast<wchar_t>(((in[i] & 0x0F) << 12) | ((in[i + 1] & 0x3F) << 6) | (in[i + 2] & 0x3F)));
                    i += 3;
                }
                else {
                    throw std::invalid_argument("Invalid UTF-8 sequence");
                }
            }
            else if ((in[i] & 0xF8) == 0xF0) {
                // Checking for a valid utf8 sequence
                if (i + 3 < in.size() && (in[i + 1] & 0xC0) == 0x80 && (in[i + 2] & 0xC0) == 0x80 && (in[i + 3] & 0xC0) == 0x80) {
#ifdef _WIN32
                    uint32_t u32 = ((in[i] & 0x07) << 24) | ((in[i + 1] & 0x3F) << 12) | ((in[i + 2] & 0x3F) << 6) | (in[i + 3] & 0x3F);
                    out.push_back(static_cast<wchar_t>(((u32 - 0x10000) >> 10) + 0xD800));
                    out.push_back(static_cast<wchar_t>((u32 % 0x400) + 0xDC00));
#else
                    out.push_back(static_cast<wchar_t>(((in[i] & 0x07) << 24) | ((in[i + 1] & 0x3F) << 12) | ((in[i + 2] & 0x3F) << 6) | (in[i + 3] & 0x3F)));
#endif
                    i += 4;
                }
                else {
                    throw std::invalid_argument("Invalid UTF-8 sequence");
                }
            }
            else {
                throw std::invalid_argument("Invalid character");
                break;
            }
        }
        return out;
    }
}

