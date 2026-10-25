#pragma once
#include <memory>
#include <vector>

#include <vk_mem_alloc_raii.hpp>

// MSVC doesn't supress C4702. This is the compiler bug:
// https://developercommunity.visualstudio.com/t/error:-C4702-with-external:w0/1696694
#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif
