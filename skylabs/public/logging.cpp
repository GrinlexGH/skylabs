#include <skylabs/public/logging.hpp>

#ifdef PLATFORM_ANDROID
#include <SDL3/SDL_log.h>
#endif

namespace Log {
std::atomic<Level> g_runtimeLevel{Level::eTrace};

void SubmitLog(const Level level, const std::source_location& loc, const std::string& message) {
#ifdef PLATFORM_ANDROID
    const char* msg = log.text.c_str();
    switch (log.level) {
        case Log::Level::eFatal: SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS: %s", msg); break;
        case Log::Level::eError: SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS: %s", msg); break;
        case Log::Level::eWarning: SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS: %s", msg); break;
        case Log::Level::eInfo: SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS: %s", msg); break;
        case Log::Level::eDebug: SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS: %s", msg); break;
        case Log::Level::eVerbose: SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS: %s", msg); break;
        case Log::Level::eTrace: SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS: (TRACE) %s", msg); break;
        default: break;
    }
#else
    struct LogConfig {
        std::string_view name;
        fmt::text_style style;
    };

    constexpr frozen::unordered_map<Level, LogConfig, 7> levelConfigs = {{
        { Level::eFatal,   { "Fatal",   fmt::fg(fmt::color::dark_red) } },
        { Level::eError,   { "Error",   fmt::fg(fmt::rgb(204, 0, 0)) } },
        { Level::eWarning, { "Warning", fmt::fg(fmt::rgb(196, 160, 0)) } },
        { Level::eInfo,    { "Info",    fmt::fg(fmt::rgb(114, 159, 207)) } },
        { Level::eDebug,   { "Debug",   fmt::fg(fmt::rgb(168, 228, 160)) } },
        { Level::eVerbose, { "Verbose", fmt::fg(fmt::color::dark_gray) } },
        { Level::eTrace,   { "Trace",   fmt::fg(fmt::color::blue) } }
    }};

    if (levelConfigs.contains(level)) {
        const auto& [name, style] = levelConfigs.at(level);
        fmt::print(style, "[{}] {}: ", name, loc.file_name());
    } else {
        fmt::print("[Unknown] ");
    }

    fmt::println("{}", message);
#endif

    if (level == Level::eFatal) {
        std::fflush(stdout);
        std::abort();
    }
}
}
