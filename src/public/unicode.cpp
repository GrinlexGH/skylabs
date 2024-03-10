#include "unicode.hpp"

enum class utf8_conversion_exception::ConversionType {
    FromUtf8ToUtf16 = 0,
    FromUtf16ToUtf8
};

inline utf8_conversion_exception::utf8_conversion_exception(
    const char* const message,
    const uint32_t errorCode,
    const ConversionType type)

    : std::runtime_error(message),
    _errorCode(errorCode),
    _conversionType(type)
{ }

inline utf8_conversion_exception::utf8_conversion_exception(
    const std::string& message,
    const uint32_t errorCode,
    const ConversionType type)

    : std::runtime_error(message)
    , _errorCode(errorCode)
    , _conversionType(type)
{ }

inline uint32_t utf8_conversion_exception::ErrorCode() const {
    return _errorCode;
}

inline utf8_conversion_exception::ConversionType utf8_conversion_exception::Direction() const {
    return _conversionType;
}


#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

inline std::wstring utf8_to_utf16(const std::string& utf8) {
    std::wstring utf16;

    if (utf8.empty()) {
        return utf16;
    }

    constexpr DWORD kFlags = MB_ERR_INVALID_CHARS;

    if (utf8.length() > static_cast<size_t>((std::numeric_limits<int>::max)())) {
        throw std::overflow_error(
            "Input string too long: size_t-length doesn't fit into int.");
    }
    const int utf8Length = static_cast<int>(utf8.length());

    const int utf16Length = ::MultiByteToWideChar(
        CP_UTF8,
        kFlags,
        utf8.data(),
        utf8Length,
        nullptr,
        0
    );
    if (utf16Length == 0) {
        const DWORD error = ::GetLastError();
        throw utf8_conversion_exception(
            error == ERROR_NO_UNICODE_TRANSLATION ?
            "Invalid UTF-8 sequence found in input string."
            :
            "Cannot get result string length when converting " \
            "from UTF-8 to UTF-16 (MultiByteToWideChar failed).",
            error,
            utf8_conversion_exception::ConversionType::FromUtf8ToUtf16);
    }

    utf16.resize(utf16Length);

    int result = ::MultiByteToWideChar(
        CP_UTF8,
        kFlags,
        utf8.data(),
        utf8Length,
        &utf16[0],
        utf16Length
    );
    if (result == 0) {
        const DWORD error = ::GetLastError();
        throw utf8_conversion_exception(
            error == ERROR_NO_UNICODE_TRANSLATION ?
            "Invalid UTF-8 sequence found in input string."
            :
            "Cannot convert from UTF-8 to UTF-16 "\
            "(MultiByteToWideChar failed).",
            error,
            utf8_conversion_exception::ConversionType::FromUtf8ToUtf16);
    }

    return utf16;
}

inline std::string utf16_to_utf8(const std::wstring& utf16) {
    std::string utf8;

    if (utf16.empty()) {
        return utf8;
    }

    constexpr DWORD kFlags = WC_ERR_INVALID_CHARS;

    if (utf16.length() > static_cast<size_t>((std::numeric_limits<int>::max)())) {
        throw std::overflow_error(
            "Input string too long: size_t-length doesn't fit into int.");
    }
    const int utf16Length = static_cast<int>(utf16.length());

    const int utf8Length = ::WideCharToMultiByte(
        CP_UTF8,
        kFlags,
        utf16.data(),
        utf16Length,
        nullptr,
        0,
        nullptr, nullptr
    );
    if (utf8Length == 0) {
        const DWORD error = ::GetLastError();
        throw utf8_conversion_exception(
            error == ERROR_NO_UNICODE_TRANSLATION ?
            "Invalid UTF-16 sequence found in input string."
            :
            "Cannot get result string length when converting "\
            "from UTF-16 to UTF-8 (WideCharToMultiByte failed).",
            error,
            utf8_conversion_exception::ConversionType::FromUtf16ToUtf8);
    }

    utf8.resize(utf8Length);

    int result = ::WideCharToMultiByte(
        CP_UTF8,
        kFlags,
        utf16.data(),
        utf16Length,
        &utf8[0],
        utf8Length,
        nullptr, nullptr
    );
    if (result == 0) {
        const DWORD error = ::GetLastError();
        throw utf8_conversion_exception(
            error == ERROR_NO_UNICODE_TRANSLATION ?
            "Invalid UTF-16 sequence found in input string."
            :
            "Cannot convert from UTF-16 to UTF-8 "\
            "(WideCharToMultiByte failed).",
            error,
            utf8_conversion_exception::ConversionType::FromUtf16ToUtf8);
    }

    return utf8;
}

#endif

