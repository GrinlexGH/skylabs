#pragma once
#include <iostream>
#include <mutex>
#include <array>
#include <atomic>
#include <tuple>
#include <string_view>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <stdexcept>
#include <source_location>
#include <variant>

#include <vulkan/vulkan.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <boost/range/irange.hpp>
#include <boost/nowide/convert.hpp>

// MSVC doesn't supress C4702. This is the compiler bug:
// https://developercommunity.visualstudio.com/t/error:-C4702-with-external:w0/1696694
#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4702)
#endif

#include <fmt/format.h>
#include <fmt/color.h>
#include <fmt/ranges.h>

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif

#include <frozen/unordered_map.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

#include <skylabs/public/dll_export.hpp>
