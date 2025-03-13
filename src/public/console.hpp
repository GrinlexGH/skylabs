#pragma once
#include "stc.hpp"

#include <iostream>
#include <format>

enum CLogType : std::uint8_t
{
    eDebug = 0,
    eInfo,
    eWarn,
    eErr
};

template <typename... Args>
void Log(const CLogType type, const std::format_string<Args...> fmt, Args&&... args) {
    std::cout << '[';
    switch (type) {
        case eDebug: {
            std::cout << stc::rgb_fg(168, 228, 160) << "Debug" << stc::reset_fg << "] ";
        } break;
        case eInfo: {
            std::cout << stc::rgb_fg(114, 159, 207) << "Info" << stc::reset_fg << "] ";
        } break;
        case eWarn: {
            std::cout << stc::rgb_fg(196, 160, 0) << "Warning" << stc::reset_fg << "] ";
        } break;
        case eErr: {
            std::cout << stc::rgb_fg(204, 0, 0) << "Error" << stc::reset_fg << "] ";
        } break;
    }
    std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
};

#define MsgD(...) Log(CLogType::eDebug, __VA_ARGS__)
#define Msg(...) Log(CLogType::eInfo, __VA_ARGS__)
#define MsgW(...) Log(CLogType::eWarn, __VA_ARGS__)
#define MsgE(...) Log(CLogType::eErr, __VA_ARGS__)
