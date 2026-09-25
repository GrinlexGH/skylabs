#include <SDL3/SDL.h>

#include "skylabs/engine/sdl/log_sink.hpp"

namespace sk::sdl {
void LogSink::Write(const log::Category category, const log::Level level,
                    const std::source_location& /*loc*/, const std::string& message) {
    const char* msg = message.c_str();
    const std::string_view categoryTag = utils::ToString(category);
    switch (level) {
        case log::Level::eFatal:
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                            static_cast<int>(categoryTag.size()), categoryTag.data(), msg);
            break;
        case log::Level::eError:
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                         static_cast<int>(categoryTag.size()), categoryTag.data(), msg);
            break;
        case log::Level::eWarning:
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                        static_cast<int>(categoryTag.size()), categoryTag.data(), msg);
            break;
        case log::Level::eInfo:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                        static_cast<int>(categoryTag.size()), categoryTag.data(), msg);
            break;
        case log::Level::eDebug:
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                         static_cast<int>(categoryTag.size()), categoryTag.data(), msg);
            break;
        case log::Level::eVerbose:
            SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                           static_cast<int>(categoryTag.size()), categoryTag.data(), msg);
            break;
        case log::Level::eTrace:
            SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s (TRACE): %s",
                           static_cast<int>(categoryTag.size()), categoryTag.data(), msg);
            break;
        default:
            break;
    }
}
}
