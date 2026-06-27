module;
export module skylabs.pub.utils;
export import std;

export namespace Utils {
struct Extent2D {
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
};

template<std::integral T>
auto Range(T start, T end) {
    return std::views::iota(start, end);
}

template<std::integral T>
auto Range(T end) {
    return Range(static_cast<T>(0), end);
}

template<std::integral T, std::integral U>
auto Range(T start, T end, U step = 1) {
    // TODO: use std::stride on clang 23 release
    auto count = end > start ? (end - start + step - 1) / step : 0;
    return std::views::iota(static_cast<std::size_t>(0), static_cast<std::size_t>(count))
         | std::views::transform([start, step](std::size_t i) {
               return static_cast<T>(start + i * step);
           });
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
