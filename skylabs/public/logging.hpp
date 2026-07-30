#pragma once
#include <skylabs/public/pch.hpp>

namespace Log {
enum class Level : std::int8_t {
    eFatal = 0,
    eError,
    eWarning,
    eInfo,
    eDebug,
    eVerbose,
    eTrace
};

PUBLIC_GLOBAL std::atomic<Level> g_runtimeLevel;

inline bool ShouldLog(const Level level) { return level <= g_runtimeLevel.load(std::memory_order_relaxed); }
inline void SetRuntimeLevel(const Level level) { g_runtimeLevel.store(level, std::memory_order_relaxed); }
PUBLIC_CLASS void SubmitLog(Level level, const std::source_location& loc, const std::string& message);

template <typename... Args>
struct Format {
    fmt::format_string<Args...> str;
    std::source_location loc;

    template <typename T>
    consteval Format(const T& s, const std::source_location& l = std::source_location::current()) noexcept
        : str(s), loc(l) {}
};

template <typename... Args>
void DoLog(const Level level, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    if (!ShouldLog(level)) return;
    SubmitLog(level, fmt.loc, fmt::format(fmt.str, std::forward<Args>(args)...));
}

template <typename... Args>
void Fatal(Format<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eFatal, fmt, std::forward<Args>(args)...); }

template <typename... Args>
void Error(Format<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eError, fmt, std::forward<Args>(args)...); }

template <typename... Args>
void Warning(Format<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eWarning, fmt, std::forward<Args>(args)...); }

template <typename... Args>
void Info(Format<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eInfo, fmt, std::forward<Args>(args)...); }

template <typename... Args>
void Debug(Format<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eDebug, fmt, std::forward<Args>(args)...); }

template <typename... Args>
void Verbose(Format<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eVerbose, fmt, std::forward<Args>(args)...); }

template <typename... Args>
void Trace(Format<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eTrace, fmt, std::forward<Args>(args)...); }
}
