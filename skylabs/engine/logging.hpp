#pragma once
#include <memory>
#include <string_view>

#include <fmt/format.h>

namespace sk::log {
enum class Level : std::int8_t { eFatal = 0, eError, eWarning, eInfo, eDebug, eVerbose, eTrace };
enum class Category : std::uint8_t { eGeneral = 0, eVulkan };

class ISink {
public:
    virtual ~ISink() = default;
    virtual void Write(Category category, Level level, const std::string& message) = 0;
};

class ConsoleSink final : public ISink {
public:
    ~ConsoleSink() override = default;
    void Write(Category category, Level level, const std::string& message) override;
};

void AddSink(std::unique_ptr<ISink> sink);
void SubmitLog(Category category, Level level, const std::string& message);

template <typename... Args>
void DoLog(const Category category, const Level level, fmt::format_string<Args...> fmt, Args&&... args) {
    SubmitLog(category, level, fmt::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Fatal(const Category category, fmt::format_string<Args...> fmt, Args&&... args) {
    DoLog(category, Level::eFatal, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Fatal(fmt::format_string<Args...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eFatal, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Error(const Category category, fmt::format_string<Args...> fmt,
           Args&&... args) {
    DoLog(category, Level::eError, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Error(fmt::format_string<Args...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eError, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Warning(const Category category, fmt::format_string<Args...> fmt,
             Args&&... args) {
    DoLog(category, Level::eWarning, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Warning(fmt::format_string<Args...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eWarning, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Info(const Category category, fmt::format_string<Args...> fmt,
          Args&&... args) {
    DoLog(category, Level::eInfo, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Info(fmt::format_string<Args...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eInfo, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Debug(const Category category, fmt::format_string<Args...> fmt,
           Args&&... args) {
    DoLog(category, Level::eDebug, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Debug(fmt::format_string<Args...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eDebug, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Verbose(const Category category, fmt::format_string<Args...> fmt,
             Args&&... args) {
    DoLog(category, Level::eVerbose, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Verbose(fmt::format_string<Args...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eVerbose, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Trace(const Category category, fmt::format_string<Args...> fmt,
           Args&&... args) {
    DoLog(category, Level::eTrace, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Trace(fmt::format_string<Args...> fmt, Args&&... args) {
    DoLog(Category::eGeneral, Level::eTrace, fmt, std::forward<Args>(args)...);
}
}

namespace sk::utils {
std::string_view ToString(log::Level level);
std::string_view ToString(log::Category category);
}
