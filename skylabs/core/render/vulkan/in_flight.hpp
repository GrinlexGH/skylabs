#pragma once
#include <cassert>
import skylabs.vulkan.context;

namespace Vulkan {
class CInFlightContext {
public:
    CInFlightContext() = default;
    explicit CInFlightContext(const std::size_t count) : m_frameCount(count) {}

    [[nodiscard]] unsigned int InFlightIndex() const { return m_inFlightIndex; }
    [[nodiscard]] std::size_t FrameCount() const { return m_frameCount; }

    void NextFrame() { m_inFlightIndex = (m_inFlightIndex + 1) % m_frameCount; }

private:
    unsigned int m_inFlightIndex = 0;
    std::size_t m_frameCount = 1;
};

template <typename T>
class InFlight {
public:
    explicit InFlight(std::nullptr_t) : m_context(nullptr) {}

    InFlight(const CInFlightContext& context, std::vector<T>&& data) : m_context(&context), m_data(std::move(data)) {
        assert(m_data.size() == context.FrameCount() && "Container size must match frame count");
    }

    template <typename... Args>
    explicit InFlight(const CInFlightContext& context, Args&&... args) : m_context(&context) {
        m_data.reserve(context.FrameCount());
        for (std::size_t i = 0; i < context.FrameCount(); ++i) {
            m_data.emplace_back(std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    void Emplace(const CInFlightContext& context, Args&&... args) {
        m_context = &context;
        m_data.clear();
        m_data.reserve(context.FrameCount());
        for (std::size_t i = 0; i < context.FrameCount(); ++i) {
            m_data.emplace_back(std::forward<Args>(args)...);
        }
    }

    auto begin() { return m_data.begin(); }
    auto end() { return m_data.end(); }
    auto begin() const { return m_data.begin(); }
    auto end() const { return m_data.end(); }

    constexpr decltype(auto) Get() { return m_data[m_context->InFlightIndex()]; }
    constexpr decltype(auto) operator[](std::size_t index) { return m_data[index]; }
    constexpr decltype(auto) Get() const { return m_data[m_context->InFlightIndex()]; }
    constexpr decltype(auto) operator[](std::size_t index) const { return m_data[index]; }

    constexpr const T* Data() const { return m_data.data(); }
    constexpr std::size_t Size() const { return m_data.size(); }

private:
    const CInFlightContext* m_context;
    std::vector<T> m_data;
};
}
