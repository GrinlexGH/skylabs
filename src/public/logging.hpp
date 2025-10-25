#pragma once
#include <format>
#include <iostream>
#include <mutex>
#include <array>

#include <stc.hpp>
#include <tuple>

#ifdef PLATFORM_ANDROID
    #include <SDL3/SDL.h>
#endif

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
void Log(const Type type, const std::format_string<Args...> fmt, Args&&... args) {
    if (type < g_minLogLevel)
        return;

    #ifdef PLATFORM_ANDROID

    const std::scoped_lock lock(g_mutex);
    switch (type) {
        case Type::eDebug:
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "\033[38;2;168;228;160m%s\033[0m", std::format(fmt, std::forward<Args>(args)...).c_str());
            break;
        case Type::eInfo:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "\033[38;2;114;159;207m%s\033[0m", std::format(fmt, std::forward<Args>(args)...).c_str());
            break;
        case Type::eWarning:
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "\033[38;2;196;160;0m%s\033[0m", std::format(fmt, std::forward<Args>(args)...).c_str());
            break;
        case Type::eError:
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "\033[38;2;204;0;0m%s\033[0m", std::format(fmt, std::forward<Args>(args)...).c_str());
            break;
        default:
            SDL_Log("%s", std::format(fmt, std::forward<Args>(args)...).c_str());
            break;
    }

    #else

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
              << std::format(fmt, std::forward<Args>(args)...)
              << suffix
              << std::endl;

    #endif
}

template <class... Args>
void Debug(const std::format_string<Args...> fmt, Args&&... args) {
    Log(Type::eDebug, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Info(const std::format_string<Args...> fmt, Args&&... args) {
    Log(Type::eInfo, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Warning(const std::format_string<Args...> fmt, Args&&... args) {
    Log(Type::eWarning, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Error(const std::format_string<Args...> fmt, Args&&... args) {
    Log(Type::eError, fmt, std::forward<Args>(args)...);
}
}
