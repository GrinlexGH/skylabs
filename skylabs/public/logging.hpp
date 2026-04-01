#pragma once
#include <skylabs/public/pch.hpp>

namespace Log {
enum class Type : std::int8_t
{
    eDebug = 0,
    eInfo,
    eWarning,
    eError,
    eCount
};

PUBLIC_CLASS void Log(Type type, const std::string& str);

template <typename... Args>
void Log(const Type type, const fmt::format_string<Args...>& fmt, Args&&... args) {
    Log(type, fmt::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void Debug(const fmt::format_string<Args...>& fmt, Args&&... args) {
    Log(Type::eDebug, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Info(const fmt::format_string<Args...>& fmt, Args&&... args) {
    Log(Type::eInfo, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Warning(const fmt::format_string<Args...>& fmt, Args&&... args) {
    Log(Type::eWarning, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void Error(const fmt::format_string<Args...>& fmt, Args&&... args) {
    Log(Type::eError, fmt, std::forward<Args>(args)...);
}
}
