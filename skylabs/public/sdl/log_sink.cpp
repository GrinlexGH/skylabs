#include <skylabs/public/sdl/log_sink.hpp>

namespace SDL {
void CLogSink::Write(
    const Log::Category category,
    const Log::Level level,
    const std::source_location& /*loc*/,
    const std::string& message
) {
    const char* msg = message.c_str();
    const std::string_view categoryTag = Utils::ToString(category);
    switch (level) {
        case Log::Level::eFatal:
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                static_cast<int>(categoryTag.size()), categoryTag.data(), msg
            ); break;
        case Log::Level::eError:
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                static_cast<int>(categoryTag.size()), categoryTag.data(), msg
            ); break;
        case Log::Level::eWarning:
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                static_cast<int>(categoryTag.size()), categoryTag.data(), msg
            ); break;
        case Log::Level::eInfo:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                static_cast<int>(categoryTag.size()), categoryTag.data(), msg
            ); break;
        case Log::Level::eDebug:
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                static_cast<int>(categoryTag.size()), categoryTag.data(), msg
            ); break;
        case Log::Level::eVerbose:
            SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s: %s",
                static_cast<int>(categoryTag.size()), categoryTag.data(), msg
            ); break;
        case Log::Level::eTrace:
            SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS %.*s (TRACE): %s",
                static_cast<int>(categoryTag.size()), categoryTag.data(), msg
            ); break;
        default: break;
    }
}
}
