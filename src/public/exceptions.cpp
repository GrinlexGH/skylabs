#include <string>
#include <bit>
#include "charconverters.hpp"
#include "exceptions.hpp"

localized_exception::localized_exception(const std::wstring_view msg) noexcept : message(CharConverters::WideStrToUTF8<std::string>(msg))
 { }
localized_exception::localized_exception(const std::u8string_view msg) noexcept : message(std::bit_cast<const char*>(msg.data()))
 { }
localized_exception::localized_exception(const std::string_view msg) noexcept : message(msg)
 { }

const char* localized_exception::what() const noexcept {
    return message.c_str();
}


current_func_exception::current_func_exception(const std::wstring_view msg) noexcept : message(CurrentFunction + ": " + (CharConverters::WideStrToUTF8<std::string>(msg)))
{ }
current_func_exception::current_func_exception(const std::u8string_view msg) noexcept : message(CurrentFunction + ": " + (std::bit_cast<const char*>(msg.data())))
{ }
current_func_exception::current_func_exception(const std::string_view msg) noexcept : message(CurrentFunction + ": " + std::string(msg))
{ }

