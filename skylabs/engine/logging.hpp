#pragma once
#include <memory>
#include <source_location>
#include <string_view>

#include <fmt/format.h>

namespace sk::log {
enum class Level : std::int8_t { eFatal = 0, eError, eWarning, eInfo, eDebug, eVerbose, eTrace };
enum class Category : std::uint8_t { eGeneral = 0, eVulkan };

class ISink {
public:
    virtual ~ISink() = default;
    virtual void Write(Category category, Level level, const std::source_location& loc,
                       const std::string& message) = 0;
};

class ConsoleSink final : public ISink {
public:
    ~ConsoleSink() override = default;
    void Write(Category category, Level level, const std::source_location& loc,
               const std::string& message) override;
};

void AddSink(std::unique_ptr<ISink> sink);
void SubmitLog(Category category, Level level, const std::source_location& loc,
               const std::string& message);

template <typename... Args>
struct Format {
    fmt::format_string<Args...> format;
    std::source_location sourceLocation;

    // Intellisense doesn't understand concepts correctly... (29.08.2026)
#ifdef __INTELLISENSE__
    template <typename T,
              std::enable_if_t<std::is_constructible_v<fmt::format_string<Args...>, const T&>, int> = 0>
#else
    template <std::convertible_to<fmt::format_string<Args...> > T>
#endif
    // NOLINTNEXTLINE
    consteval Format(const T& format, const std::source_location& sourceLocation =
                                          std::source_location::current()) noexcept
        : format(format), sourceLocation(sourceLocation) {
    }
};

template <typename... Args>
void DoLog(const Category category, const Level level, Format<std::type_identity_t<Args>...> fmt,
           Args&&... args) {
    SubmitLog(category, level, fmt.sourceLocation, fmt::format(fmt.format, std::forward<Args>(args)...));
}

template <typename... Args>
void Fatal(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eFatal, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Fatal(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eFatal, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Error(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eError, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Error(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eError, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Warning(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eWarning, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Warning(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eWarning, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Info(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eInfo, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Info(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eInfo, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Debug(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eDebug, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Debug(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eDebug, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Verbose(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eVerbose, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Verbose(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eVerbose, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Trace(const Category category, Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(category, Level::eTrace, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Trace(Format<std::type_identity_t<Args>...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eTrace, fmt, std::forward<Args>(args)...);
}
}

namespace sk::utils {
std::string_view ToString(log::Level level);
std::string_view ToString(log::Category category);
}
