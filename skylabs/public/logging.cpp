#include <skylabs/public/logging.hpp>

namespace {
std::mutex g_mutex;
}

#ifdef PLATFORM_ANDROID
#include <SDL3/SDL_log.h>

namespace Log {
void Log(Type type, const std::string& str) {
    const std::scoped_lock lock(g_mutex);
    switch (type) {
        case Type::eDebug:
        case Type::eInfo: SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS LOG: %s", str.c_str()); break;
        case Type::eWarning: SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS LOG: %s", str.c_str()); break;
        case Type::eError: SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SKYLABS LOG: %s", str.c_str()); break;
        default: std::unreachable();
    }
}
}

#else

namespace {
constexpr std::array<std::tuple<std::string_view, int, int, int>, static_cast<std::size_t>(Log::Type::eCount)> logInfo = { {
    { "Debug", 168, 228, 160 },
    { "Info", 114, 159, 207 },
    { "Warning", 196, 160, 0 },
    { "Error", 204, 0, 0 },
} };
}

namespace Log {
void Log(Type type, const std::string& str) {
    auto [label, r, g, b] = logInfo[static_cast<std::size_t>(type)];

    const std::scoped_lock lock(g_mutex);
    std::cout << stc::true_color
              << '[' << stc::rgb_fg(r, g, b) << label << stc::reset_fg << "] "
              << str << std::endl;
}
}

#endif
