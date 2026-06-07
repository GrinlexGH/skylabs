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

inline std::atomic<Level> g_runtimeLevel { Level::eTrace };

inline bool ShouldLog(Level level) {
    return static_cast<std::int8_t>(level) <= static_cast<std::int8_t>(g_runtimeLevel.load(std::memory_order_relaxed));
}

inline void SetRuntimeLevel(Level level) {
    g_runtimeLevel.store(level, std::memory_order_relaxed);
}

void SubmitLog(Level level, const std::string& message);
}

#define SKY_LOG_BASE(level, fmt_str, ...) \
    do { \
        if (Log::ShouldLog(level)) { \
            Log::SubmitLog(level, fmt::format(fmt_str, ##__VA_ARGS__)); \
        } \
    } while (0)

#define SKY_LOG_FATAL(fmt_str, ...) SKY_LOG_BASE(Log::Level::eFatal, fmt_str, ##__VA_ARGS__)
#define SKY_LOG_ERROR(fmt_str, ...) SKY_LOG_BASE(Log::Level::eError, fmt_str, ##__VA_ARGS__)
#define SKY_LOG_WARN(fmt_str, ...) SKY_LOG_BASE(Log::Level::eWarning, fmt_str, ##__VA_ARGS__)
#define SKY_LOG_INFO(fmt_str, ...) SKY_LOG_BASE(Log::Level::eInfo, fmt_str, ##__VA_ARGS__)
#define SKY_LOG_DEBUG(fmt_str, ...) SKY_LOG_BASE(Log::Level::eDebug, fmt_str, ##__VA_ARGS__)
#define SKY_LOG_VERBOSE(fmt_str, ...) SKY_LOG_BASE(Log::Level::eVerbose, fmt_str, ##__VA_ARGS__)
#define SKY_LOG_TRACE(fmt_str, ...) SKY_LOG_BASE(Log::Level::eTrace, fmt_str, ##__VA_ARGS__)
