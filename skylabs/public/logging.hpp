#pragma once
#include <iostream>
#include <mutex>
#include <array>
#include <tuple>

#include <stc.hpp>
#include <fmt/format.h>

namespace Log {
enum class Type : std::int8_t
{
    eDebug = 0,
    eInfo,
    eWarning,
    eError,

    eCount
};

inline auto g_minLogLevel = Type::eDebug;
inline std::mutex g_mutex;

template <typename... Args>
void Log(const Type type, const fmt::format_string<Args...>& fmt, Args&&... args) {
    if (type < g_minLogLevel)
        return;

    constexpr std::array<std::tuple<std::string_view, int, int, int>, static_cast<std::size_t>(Type::eCount)> logInfo = { {
        { "Debug", 168, 228, 160 },
        { "Info", 114, 159, 207 },
        { "Warning", 196, 160, 0 },
        { "Error", 204, 0, 0 },
    } };

    auto [label, r, g, b] = logInfo[static_cast<std::size_t>(type)];

    std::string_view prefix;
    std::string_view suffix;

    switch (type) {
        case Type::eWarning:
            prefix = "\n";
            suffix = "\n";
            break;
        case Type::eError:
            prefix = "\n\n";
            suffix = "\n\n";
            break;
        default:
            break;
    }

    const std::scoped_lock lock(g_mutex);
    std::cout << prefix
              << stc::true_color
              << '[' << stc::rgb_fg(r, g, b) << label << stc::reset_fg << "] "
              << fmt::format(fmt, std::forward<Args>(args)...)
              << suffix
              << std::endl;
}

template <class... Args>
void Debug(const fmt::format_string<Args...>& fmt, Args&&... args) {
    Log(Type::eDebug, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Info(const fmt::format_string<Args...>& fmt, Args&&... args) {
    Log(Type::eInfo, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Warning(const fmt::format_string<Args...>& fmt, Args&&... args) {
    Log(Type::eWarning, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Error(const fmt::format_string<Args...>& fmt, Args&&... args) {
    Log(Type::eError, fmt, std::forward<Args>(args)...);
}
}
