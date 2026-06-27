export module skylabs.pub.logging;
import fmt;
import std;

export namespace Log {
enum class Level : std::int8_t {
    eFatal = 0,
    eError,
    eWarning,
    eInfo,
    eDebug,
    eVerbose,
    eTrace
};
}

namespace Log {
template <typename... Args>
struct FormatWithLocation {
    fmt::format_string<Args...> str;
    std::source_location loc;

    template <typename T>
    consteval FormatWithLocation(const T& s, const std::source_location l = std::source_location::current()) noexcept
        : str(s), loc(l) {}
};

inline std::atomic g_runtimeLevel { Level::eTrace };

inline bool ShouldLog(const Level level) {
    return level <= g_runtimeLevel.load(std::memory_order_relaxed);
}

void SubmitLog(Level level, const std::source_location& loc, const std::string& message);

template <typename... Args>
void DoLog(const Level level, FormatWithLocation<std::type_identity_t<Args>...> fmt, Args&&... args) {
    if (!ShouldLog(level)) return;
    SubmitLog(level, fmt.loc, fmt::format(fmt.str, std::forward<Args>(args)...));
}
}

export namespace Log {
inline void SetRuntimeLevel(const Level level) {
    g_runtimeLevel.store(level, std::memory_order_relaxed);
}

template <typename... Args> void Fatal(FormatWithLocation<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eFatal, fmt, std::forward<Args>(args)...); }
template <typename... Args> void Error(FormatWithLocation<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eError, fmt, std::forward<Args>(args)...); }
template <typename... Args> void Warning(FormatWithLocation<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eWarning, fmt, std::forward<Args>(args)...); }
template <typename... Args> void Info(FormatWithLocation<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eInfo, fmt, std::forward<Args>(args)...); }
template <typename... Args> void Debug(FormatWithLocation<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eDebug, fmt, std::forward<Args>(args)...); }
template <typename... Args> void Verbose(FormatWithLocation<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eVerbose, fmt, std::forward<Args>(args)...); }
template <typename... Args> void Trace(FormatWithLocation<std::type_identity_t<Args>...> fmt, Args&&... args) { DoLog(Level::eTrace, fmt, std::forward<Args>(args)...); }
}
