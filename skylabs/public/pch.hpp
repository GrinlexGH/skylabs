#pragma once
#include <vulkan/vulkan.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <boost/range/irange.hpp>

#include <fmt/format.h>
#include <fmt/color.h>
#include <fmt/ranges.h>

#include <frozen/unordered_map.h>

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

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

#include <skylabs/public/dll_export.hpp>
