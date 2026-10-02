#include <chrono>
#include <cstdio>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/color.h>

#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/os.hpp"

namespace {
std::vector<std::unique_ptr<sk::log::ISink>> g_sinks;

constexpr fmt::text_style GetLevelStyle(sk::log::Level level) noexcept {
    switch (level) {
        case sk::log::Level::eFatal:
            return fmt::fg(fmt::color::dark_red);
        case sk::log::Level::eError:
            return fmt::fg(fmt::rgb(204, 0, 0));
        case sk::log::Level::eWarning:
            return fmt::fg(fmt::rgb(196, 160, 0));
        case sk::log::Level::eInfo:
            return fmt::fg(fmt::rgb(114, 159, 207));
        case sk::log::Level::eDebug:
            return fmt::fg(fmt::rgb(168, 228, 160));
        case sk::log::Level::eVerbose:
            return fmt::fg(fmt::color::dark_gray);
        case sk::log::Level::eTrace:
            return fmt::fg(fmt::color::blue);
        default:
            return { };
    }
}

constexpr fmt::text_style GetCategoryStyle(sk::log::Category level) noexcept {
    switch (level) {
        case sk::log::Category::eGeneral:
            return fmt::fg(fmt::color::white_smoke);
        case sk::log::Category::eVulkan:
            return fmt::fg(fmt::color::indian_red);
        default:
            return { };
    }
}

std::string FormatTzOffset(const std::array<char, 6>& tz) {
    if (tz[0] == '\0') {
        return "";
    }

    return fmt::format(" {:.3}:{}", tz.data(), tz.data() + 3);
}
}

namespace sk::log {
void ConsoleSink::Write(const Category category, const Level level, const std::source_location& /*loc*/,
                        const std::string& message) {
    const auto now = std::chrono::system_clock::now();
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    const os::LocalTime localTime = sk::os::GetLocalTime(now);

    fmt::println(
        "{} {} {}: {}",
        fmt::styled(
            fmt::format("[{:%H:%M:%S}.{:03d}{}] ", localTime.tm, ms, FormatTzOffset(localTime.tzOffset)),
            fmt::fg(fmt::color::gray)),
        fmt::styled(fmt::format("[{}]", utils::ToString(category)), GetCategoryStyle(category)),
        fmt::styled(fmt::format("[{}]", utils::ToString(level)), GetLevelStyle(level)), message);
}

void AddSink(std::unique_ptr<ISink> sink) { g_sinks.push_back(std::move(sink)); }

void SubmitLog(const Category category, const Level level, const std::source_location& loc,
               const std::string& message) {
    for (const auto& sink : g_sinks) {
        sink->Write(category, level, loc, message);
    }

    if (level == Level::eFatal) {
        std::fflush(stdout);
        std::fflush(stderr);
    }
}
}

namespace sk::utils {
std::string_view ToString(const log::Level level) {
    switch (level) {
        case log::Level::eFatal:
            return "Fatal";
        case log::Level::eError:
            return "Error";
        case log::Level::eWarning:
            return "Warning";
        case log::Level::eInfo:
            return "Info";
        case log::Level::eDebug:
            return "Debug";
        case log::Level::eVerbose:
            return "Verbose";
        case log::Level::eTrace:
            return "Trace";
        default:
            return "";
    }
}

std::string_view ToString(const log::Category category) {
    switch (category) {
        case log::Category::eGeneral:
            return "General";
        case log::Category::eVulkan:
            return "Vulkan";
        default:
            return "";
    }
}
}
