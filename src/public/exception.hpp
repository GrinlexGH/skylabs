#pragma once

#include <exception>
#include <string>

class Exception : public std::exception
{
private:
    const std::string message;
public:
    Exception(const std::wstring_view msg) noexcept;
    Exception(const std::u8string_view msg) noexcept;
    Exception(const std::string_view msg) noexcept;
    const char* what() const noexcept;
};

