#pragma once

#include <stdexcept>

class utf8_conversion_exception : public std::runtime_error
{
public:
    enum class ConversionType;

    utf8_conversion_exception(
        const char* const message,
        const uint32_t errorCode,
        const ConversionType type
    );

    utf8_conversion_exception(
        const std::string& message,
        const uint32_t errorCode,
        const ConversionType type);

    uint32_t ErrorCode() const;
    ConversionType Direction() const;

private:
    uint32_t _errorCode;
    ConversionType _conversionType;
};

#ifdef WIN32

extern inline std::wstring utf8_to_utf16(const std::string& utf8);
extern inline std::string utf16_to_utf8(const std::wstring& utf16);

#endif

