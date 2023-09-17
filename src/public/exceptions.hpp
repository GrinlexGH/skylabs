#pragma once

#include <exception>
#include <string>
#include "macros.hpp"

/**
* @brief Accepts any kind of strings and saves it in utf8.
*/
class localized_exception : public std::exception
{
private:
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
* like Person::SayHello: Cant say hello because code page is invalid.
*/
class current_func_exception : public localized_exception
{
private:
    const std::string message = CurrentFunction + ": Unknown localized exception!";
public:
    explicit current_func_exception(const std::wstring_view msg) noexcept;
    explicit current_func_exception(const std::u8string_view msg) noexcept;
    explicit current_func_exception(const std::string_view msg) noexcept;
};

