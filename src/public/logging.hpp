#pragma once
#include <format>
#include <iostream>
#include <mutex>

#include "stc.hpp"

namespace Log {
struct LogMutex {
    inline static std::mutex m_mutex;
};

enum class LogType : std::int8_t
{
    eDebug = 0,
    eInfo,
    eWarning,
    eError
};

template <class... Args>
void Log(const LogType type, const std::format_string<Args...> fmt, Args&&... args) {
    std::lock_guard<std::mutex> lock(LogMutex::m_mutex);

    std::cout << '[';
    switch (type) {
        case LogType::eDebug: {
            std::cout << stc::rgb_fg(168, 228, 160) << "Debug" << stc::reset_fg << "] ";
        } break;
        case LogType::eInfo: {
            std::cout << stc::rgb_fg(114, 159, 207) << "Info" << stc::reset_fg << "] ";
        } break;
        case LogType::eWarning: {
            std::cout << stc::rgb_fg(196, 160, 0) << "Warning" << stc::reset_fg << "] ";
        } break;
        case LogType::eError: {
            std::cout << stc::rgb_fg(204, 0, 0) << "Error" << stc::reset_fg << "] ";
        } break;
    }
    std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

template <class... Args>
void Debug(const std::format_string<Args...> fmt, Args&&... args) {
    Log(LogType::eDebug, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Info(const std::format_string<Args...> fmt, Args&&... args) {
    Log(LogType::eInfo, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Warning(const std::format_string<Args...> fmt, Args&&... args) {
    Log(LogType::eWarning, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Error(const std::format_string<Args...> fmt, Args&&... args) {
    Log(LogType::eError, fmt, std::forward<Args>(args)...);
}
}
