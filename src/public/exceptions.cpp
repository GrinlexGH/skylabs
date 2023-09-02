#include <string.h>
#include "charconverters.hpp"
#include "exceptions.hpp"

localized_exception::localized_exception(const std::wstring_view msg) noexcept : message(CharConverters::WideStrToUTF8<std::u8string_view>(msg))
 { }
localized_exception::localized_exception(const std::u8string_view msg) noexcept : message(msg)
 { }
localized_exception::localized_exception(const std::string_view msg) noexcept : message(std::bit_cast<const char8_t*>(msg.data()))
 { }

const char* localized_exception::what() const noexcept {
    return std::bit_cast<const char*>(message.c_str());
}

