#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
constexpr const auto MAX_FRAMES_IN_FLIGHT = 3;

class InFlightContext {
public:
    explicit InFlightContext(std::nullptr_t) {}
    explicit InFlightContext(std::size_t count) : m_frameCount(count) {}

    [[nodiscard]] unsigned int InFlightIndex() const { return m_inFlightIndex; }
    [[nodiscard]] std::size_t FrameCount() const { return m_frameCount; }

    void NextFrame() { if (m_frameCount > 0) m_inFlightIndex = (m_inFlightIndex + 1) % m_frameCount; }

private:
    unsigned int m_inFlightIndex = 0;
    std::size_t m_frameCount = 0;
};

template<typename F, typename T>
concept ActionOn = requires(F f, T& t) {
    { f(t) } -> std::same_as<void>;
};

template <typename T>
class InFlight {
public:
    explicit InFlight(std::nullptr_t) : m_context(nullptr) {}

    template <typename... Args>
    InFlight(const InFlightContext& context, Args&&... args) : m_context(&context) {
        m_data.reserve(context.FrameCount());
        for (std::size_t i = 0; i < context.FrameCount(); ++i) {
            m_data.emplace_back(std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    void Emplace(const InFlightContext& context, Args&&... args) {
        m_context = &context;
        m_data.clear();
        m_data.reserve(context.FrameCount());
        for (std::size_t i = 0; i < context.FrameCount(); ++i) {
            m_data.emplace_back(std::forward<Args>(args)...);
        }
    }

    template<ActionOn<T> Func>
    void Apply(Func& func) {
        for (auto& item : m_data) { func(item); }
    }

    constexpr T& Get() { return m_data[m_context->InFlightIndex()]; }
    constexpr const T& Get() const { return m_data[m_context->InFlightIndex()]; }
    constexpr T& operator[](std::size_t index) { return m_data[index]; }
    constexpr const T& operator[](std::size_t index) const { return m_data[index]; }

    constexpr std::size_t Size() const { return m_data.size(); }

private:
    const InFlightContext* m_context;
    std::vector<T> m_data;
};
}
