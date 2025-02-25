#pragma once
#include "stc.hpp"

#include <iostream>
#include <format>

enum CLogType : std::uint8_t
{
    Info = 0,
    Warn,
    Err
};

template <typename... Args>
void Log(const CLogType type, const std::format_string<Args...> fmt, Args&&... args) {
    std::cout << '[';
    switch (type) {
        case Info: {
            std::cout << stc::rgb_fg(114, 159, 207) << "Info" << stc::reset_fg << "] ";
        } break;
        case Warn: {
            std::cout << stc::rgb_fg(196, 160, 0) << "Warning" << stc::reset_fg << "] ";
        } break;
        case Err: {
            std::cout << stc::rgb_fg(204, 0, 0) << "Error" << stc::reset_fg << "] ";
        } break;
    }
    std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
};

#define Msg(...) Log(CLogType::Info, __VA_ARGS__);
#define Warning(...) Log(CLogType::Warn, __VA_ARGS__);
#define Error(...) Log(CLogType::Err, __VA_ARGS__);
