#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "unicode.hpp"

inline std::wstring Utf8ToUtf16(std::string_view utf8) {
    std::wstring utf16;

    if (utf8.empty()) {
        return utf16;
    }

    const int kUtf8Length = [utf8]() {
            if (utf8.length() > static_cast<size_t>((std::numeric_limits<int>::max)())) {
                return (std::numeric_limits<int>::max)();
            }
            else {
                return static_cast<int>(utf8.length());
            }
        }();
    const int kUtf16Length = ::MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        utf8.data(),
        kUtf8Length,
        nullptr,
        0
    );
    if (kUtf16Length == 0) {
        return utf16;
    }

    utf16.resize(kUtf16Length);
    ::MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        utf8.data(),
        kUtf8Length,
        &utf16[0],
        kUtf16Length
    );

    return utf16;
}

inline std::string Utf16ToUtf8(std::wstring_view utf16) {
    std::string utf8;

    if (utf16.empty()) {
        return utf8;
    }

    const int kUtf16Length = [utf16]() {
            if (utf16.length() > static_cast<size_t>((std::numeric_limits<int>::max)())) {
                return (std::numeric_limits<int>::max)();
            }
            else {
                return static_cast<int>(utf16.length());
            }
        }();
    const int kUtf8Length = ::WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        utf16.data(),
        kUtf16Length,
        nullptr,
        0,
        NULL, NULL) - 1;
    if (kUtf8Length == 0) {
        return utf8;
    }

    utf8.resize(kUtf8Length);
    ::WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        utf16.data(),
        kUtf16Length,
        &utf8[0],
        kUtf8Length,
        nullptr, nullptr);

    return utf8;
}

#endif

