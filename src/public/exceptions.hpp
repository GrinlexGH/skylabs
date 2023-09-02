#pragma once

#include <exception>
#include <string>

class localized_exception : public std::exception
{
private:
    const std::u8string message = u8"Unknown localized exception!";
public:
    localized_exception() noexcept = default;
    explicit localized_exception(const std::wstring_view msg) noexcept;
    explicit localized_exception(const std::u8string_view msg) noexcept;
    explicit localized_exception(const std::string_view msg) noexcept;
    ~localized_exception() noexcept override = default;
    const char* what() const noexcept override;
};

