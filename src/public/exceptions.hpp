#pragma once

#include <exception>
#include <string>
#include "macros.hpp"

/**
* @brief Accepts any kind of strings and saves it in utf8.
*/
class localized_exception : public std::exception
{
protected:
    const std::string message = "Unknown localized exception!";
public:
    localized_exception() noexcept = default;
    explicit localized_exception(const std::wstring_view msg) noexcept;
    explicit localized_exception(const std::u8string_view msg) noexcept;
    explicit localized_exception(const std::string_view msg) noexcept;
    ~localized_exception() noexcept override = default;
    const char* what() const noexcept override;
};

/**
* @brief Inherits from localized_exception and just adds current function name + message
* like `Person::SayHello: Cant say hello because code page is invalid.`
*/
class current_func_exception : public localized_exception
{
public:
    current_func_exception() noexcept = delete;
    explicit current_func_exception(std::string currentFuncName, const std::wstring_view msg) noexcept;
    explicit current_func_exception(std::string currentFuncName, const std::u8string_view msg) noexcept;
    explicit current_func_exception(std::string currentFuncName, const std::string_view msg) noexcept;
    ~current_func_exception() noexcept override = default;
    #define func_exception(msg) current_func_exception(CurrentFunction, msg)
    const char* what() const noexcept override;
};

