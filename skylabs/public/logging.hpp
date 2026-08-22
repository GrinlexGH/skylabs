#pragma once
#include <skylabs/public/pch.hpp>

namespace Log {
enum class Level : std::int8_t
{
    eFatal = 0,
    eError,
    eWarning,
    eInfo,
    eDebug,
    eVerbose,
    eTrace
};

enum class Category : std::uint8_t
{
    eGeneral = 0,
    eVulkan
};

class PUBLIC_CLASS ISink
{
public:
    virtual ~ISink() = default;
    virtual void Write(
        Category category,
        Level level,
        const std::source_location& loc,
        const std::string& message
    ) = 0;
};

PUBLIC_CLASS void AddSink(std::unique_ptr<ISink> sink);

class PUBLIC_CLASS CConsoleSink final : public ISink
{
public:
    ~CConsoleSink() override = default;
    void Write(
        Category category,
        Level level,
        const std::source_location& loc,
        const std::string& message
    ) override;
};

PUBLIC_CLASS void SubmitLog(
    Category category,
    Level level,
    const std::source_location& loc,
    const std::string& message
);

template<typename... Args>
struct Format
{
    fmt::format_string<Args...> str;
    std::source_location loc;

    template<typename T>
        requires std::convertible_to<const T&, fmt::format_string<Args...>>
    consteval Format(const T& s, const std::source_location& l = std::source_location::current()) noexcept :
        str(s), loc(l) { }
};

template<typename... Args>
void DoLog(const Category category, const Level level, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    SubmitLog(category, level, fmt.loc, fmt::format(fmt.str, std::forward<Args>(args)...));
}

template<typename... Args>
void Fatal(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eFatal, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Fatal(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eFatal, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Error(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eError, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Error(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eError, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Warning(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eWarning, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Warning(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eWarning, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Info(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eInfo, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Info(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eInfo, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Debug(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eDebug, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Debug(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eDebug, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Verbose(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eVerbose, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Verbose(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eVerbose, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Trace(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eTrace, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void Trace(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eTrace, fmt, std::forward<Args>(args)...);
}
}

namespace Utils {
inline std::string_view ToString(const Log::Level level) {
    constexpr frozen::unordered_map<Log::Level, std::string_view, 7> levelNames = {{
        { Log::Level::eFatal, "Fatal" },
        { Log::Level::eError, "Error" },
        { Log::Level::eWarning, "Warning" },
        { Log::Level::eInfo, "Info" },
        { Log::Level::eDebug, "Debug" },
        { Log::Level::eVerbose, "Verbose" },
        { Log::Level::eTrace, "Trace" }
    }};
    return levelNames.at(level);
}

inline std::string_view ToString(const Log::Category category) {
    constexpr frozen::unordered_map<Log::Category, std::string_view, 2> categoryNames = {{
        { Log::Category::eGeneral, "General" },
        { Log::Category::eVulkan, "Vulkan" }
    }};
    return categoryNames.at(category);
}
}
