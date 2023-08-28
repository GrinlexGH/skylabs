#include "charconverters.hpp"
#include "exception.hpp"

Exception::Exception(const std::wstring_view msg) noexcept : message(CharConverters::WideStrToUTF8(msg))
 { }
Exception::Exception(const std::u8string_view msg) noexcept : message(reinterpret_cast<const char*>(msg.data()))
 { }
Exception::Exception(const std::string_view msg) noexcept : message(msg)
 { }

const char* Exception::what() const noexcept {
    return message.c_str();
}

