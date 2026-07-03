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
    return boost::irange(end);
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

    CTimer() : m_start(Clock::now()) {}

    void reset() { m_start = Clock::now(); }

    auto elapsedSeconds() const {
        return std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - m_start).count();
    }

    auto elapsedMilliseconds() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - m_start).count();
    }

    auto elapsedMicroseconds() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - m_start).count();
    }

    auto elapsedNanoseconds() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - m_start).count();
    }

private:
    TimePoint m_start;
};
}
