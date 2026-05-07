#pragma once
#include <skylabs/public/pch.hpp>

namespace Utils {
struct Extent2D {
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
};

template<std::integral T>
auto Range(T start, T end) {
    return boost::irange(start, end);
}

template<std::integral T>
auto Range(T end) {
    return Range(static_cast<T>(0), end);
}

template<std::integral T, std::integral U>
auto Range(T start, T end, U step = 1) {
    return boost::irange(start, end, step);
}

class CTimer
{
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    CTimer() : m_Start(Clock::now()) {}

    void reset() { m_Start = Clock::now(); }

    double elapsedSeconds() const {
        return std::chrono::duration<double>(Clock::now() - m_Start).count();
    }

    double elapsedMilliseconds() const {
        return std::chrono::duration<double, std::milli>(Clock::now() - m_Start).count();
    }

    long long elapsedMicroseconds() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - m_Start).count();
    }

private:
    TimePoint m_Start;
};
}
