#include <cstdio>
#include <vector>

#include <fmt/color.h>
#include <frozen/unordered_map.h>

#include "skylabs/engine/logging.hpp"

namespace {
std::vector<std::unique_ptr<sk::log::ISink>> g_sinks;
}

namespace sk::log {
void ConsoleSink::Write(const Category category, const Level level, const std::source_location& /*loc*/,
                        const std::string& message) {
    constexpr frozen::unordered_map<Level, fmt::text_style, 7> levelStyles = { {
        { Level::eFatal, fmt::fg(fmt::color::dark_red) },
        { Level::eError, fmt::fg(fmt::rgb(204, 0, 0)) },
        { Level::eWarning, fmt::fg(fmt::rgb(196, 160, 0)) },
        { Level::eInfo, fmt::fg(fmt::rgb(114, 159, 207)) },
        { Level::eDebug, fmt::fg(fmt::rgb(168, 228, 160)) },
        { Level::eVerbose, fmt::fg(fmt::color::dark_gray) },
        { Level::eTrace, fmt::fg(fmt::color::blue) },
    } };

    constexpr frozen::unordered_map<Category, fmt::text_style, 2> categoryStyles = { {
        { Category::eGeneral, fmt::fg(fmt::color::white_smoke) },
        { Category::eVulkan, fmt::fg(fmt::color::indian_red) },
    } };

    fmt::print(categoryStyles.at(category), "[{}] ", utils::ToString(category));
    fmt::print(levelStyles.at(level), "[{}]: ", utils::ToString(level));
    fmt::println("{}", message);
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
    constexpr frozen::unordered_map<log::Level, std::string_view, 7> levelNames = {
        { { log::Level::eFatal, "Fatal" },
          { log::Level::eError, "Error" },
          { log::Level::eWarning, "Warning" },
          { log::Level::eInfo, "Info" },
          { log::Level::eDebug, "Debug" },
          { log::Level::eVerbose, "Verbose" },
          { log::Level::eTrace, "Trace" } }
    };
    return levelNames.at(level);
}

std::string_view ToString(const log::Category category) {
    constexpr frozen::unordered_map<log::Category, std::string_view, 2> categoryNames = {
        { { log::Category::eGeneral, "General" }, { log::Category::eVulkan, "Vulkan" } }
    };

    return categoryNames.at(category);
}
}
