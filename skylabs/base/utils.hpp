#pragma once
#include <chrono>
#include <concepts>
#include <ranges>

namespace sk::utils {
struct Extent2D {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

template <std::integral T>
constexpr auto Range(T stop) {
    return std::views::iota(T { 0 }, stop);
}

class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    Timer() : m_start(Clock::now()) { }

    void Reset() { m_start = Clock::now(); }

    auto ElapsedSeconds() const {
        return std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - m_start).count();
    }

    auto ElapsedMilliseconds() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - m_start).count();
    }

    auto ElapsedMicroseconds() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - m_start).count();
    }

    auto ElapsedNanoseconds() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - m_start).count();
    }

private:
    TimePoint m_start;
};

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
}
