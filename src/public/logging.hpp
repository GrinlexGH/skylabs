#pragma once
#include <format>
#include <iostream>

#include "stc.hpp"

namespace Log {
enum class CLogType : std::uint8_t
{
    eDebug = 0,
    eInfo,
    eWarning,
    eError
};

template <class... Args>
void Log(const CLogType type, const std::format_string<Args...> fmt, Args&&... args) {
    std::cout << '[';
    switch (type) {
        case CLogType::eDebug: {
            std::cout << stc::rgb_fg(168, 228, 160) << "Debug" << stc::reset_fg << "] ";
        } break;
        case CLogType::eInfo: {
            std::cout << stc::rgb_fg(114, 159, 207) << "Info" << stc::reset_fg << "] ";
        } break;
        case CLogType::eWarning: {
            std::cout << stc::rgb_fg(196, 160, 0) << "Warning" << stc::reset_fg << "] ";
        } break;
        case CLogType::eError: {
            std::cout << stc::rgb_fg(204, 0, 0) << "Error" << stc::reset_fg << "] ";
        } break;
    }
    std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

template <class... Args>
void Debug(const std::format_string<Args...> fmt, Args&&... args) {
    Log(CLogType::eDebug, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Info(const std::format_string<Args...> fmt, Args&&... args) {
    Log(CLogType::eInfo, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Warning(const std::format_string<Args...> fmt, Args&&... args) {
    Log(CLogType::eWarning, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Error(const std::format_string<Args...> fmt, Args&&... args) {
    Log(CLogType::eError, fmt, std::forward<Args>(args)...);
}
}
