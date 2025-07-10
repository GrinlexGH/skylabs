#pragma once
#include <format>
#include <iostream>
#include <mutex>
#include <array>

#include <stc.hpp>

namespace Log {
enum class LogType : std::int8_t
{
    eDebug = 0,
    eInfo,
    eWarning,
    eError,
    eCount
};

inline auto g_minLogLevel = LogType::eDebug;
inline std::mutex g_mutex;

template <class... Args>
void Log(const LogType type, const std::format_string<Args...> fmt, Args&&... args) {
    if (type < g_minLogLevel)
        return;

    constexpr std::array<std::tuple<std::string_view, int, int, int>, static_cast<std::size_t>(LogType::eCount)> logPrefixes = { {
        { "Debug", 168, 228, 160 },
        { "Info", 114, 159, 207 },
        { "Warning", 196, 160, 0 },
        { "Error", 204, 0, 0 },
    } };

    auto [label, r, g, b] = logPrefixes[static_cast<std::size_t>(type)];

    std::string_view prefix;
    std::string_view suffix;

    switch (type) {
        case LogType::eWarning:
            prefix = "\n";
            suffix = "\n";
            break;
        case LogType::eError:
            prefix = "\n\n";
            suffix = "\n\n";
            break;
        default:
            break;
    }

    std::lock_guard lock(g_mutex);
    std::cout << prefix
              << stc::true_color
              << '[' << stc::rgb_fg(r, g, b) << label << stc::reset_fg << "] "
              << std::format(fmt, std::forward<Args>(args)...) 
              << suffix
              << std::endl;
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
