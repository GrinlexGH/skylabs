#pragma once

#include <exception>
#include <string>
#include "macros.hpp"

/**
* @brief Accepts any kind of strings and saves it in utf8.
*/
class CLocalizedException : public std::exception
{
protected:
    const std::string message = "Unknown localized exception!";
    const int code = 0;
public:
    CLocalizedException() noexcept = default;
    explicit CLocalizedException(const std::wstring_view msg) noexcept;
    explicit CLocalizedException(const std::u8string_view msg) noexcept;
    explicit CLocalizedException(const std::string_view msg) noexcept;
    ~CLocalizedException() noexcept override = default;
    const char* what() const noexcept override;
};

/**
* @brief Inherits from CLocalizedException and adds current function name + message
* like `Person::SayHello: Cant say hello because code page is invalid.`
*/
class CCurrentFuncException : public CLocalizedException
{
public:
    CCurrentFuncException() noexcept = delete;
    explicit CCurrentFuncException(std::string currentFuncName, const std::wstring_view msg) noexcept;
    explicit CCurrentFuncException(std::string currentFuncName, const std::u8string_view msg) noexcept;
    explicit CCurrentFuncException(std::string currentFuncName, const std::string_view msg) noexcept;
    ~CCurrentFuncException() noexcept override = default;
    #define CCurrentFuncExcept(msg) CCurrentFuncException(CurrentFunction, msg)
    const char* what() const noexcept override;
};

