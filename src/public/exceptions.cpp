#include <string>
#include <bit>
#include "charconverters.hpp"
#include "exceptions.hpp"

CLocalizedException::CLocalizedException(const std::wstring_view msg) noexcept : message(CharConverters::WideStrToUTF8<std::string>(msg))
 { }
CLocalizedException::CLocalizedException(const std::u8string_view msg) noexcept : message(std::bit_cast<const char*>(msg.data()))
 { }
CLocalizedException::CLocalizedException(const std::string_view msg) noexcept : message(msg)
 { }

const char* CLocalizedException::what() const noexcept {
    return message.c_str();
}


CCurrentFuncException::CCurrentFuncException(const std::string &currentFuncName, const std::wstring_view msg) noexcept : CLocalizedException(currentFuncName + ":\n\n" + (CharConverters::WideStrToUTF8<std::string>(msg)))
{ }
CCurrentFuncException::CCurrentFuncException(const std::string &currentFuncName, const std::u8string_view msg) noexcept : CLocalizedException(currentFuncName + ":\n\n" + (std::bit_cast<const char*>(msg.data())))
{ }
CCurrentFuncException::CCurrentFuncException(const std::string &currentFuncName, const std::string_view msg) noexcept : CLocalizedException(currentFuncName + ":\n\n" + std::string(msg))
{ }

