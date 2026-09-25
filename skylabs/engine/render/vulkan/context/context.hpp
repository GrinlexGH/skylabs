#pragma once
#include "skylabs/engine/window.hpp"
#include "skylabs/engine/render/vulkan/context/allocator.hpp"
#include "skylabs/engine/render/vulkan/context/device.hpp"
#include "skylabs/engine/render/vulkan/context/instance.hpp"
#include "skylabs/engine/render/vulkan/context/surface.hpp"

namespace sk::render::vulkan {
class Context {
public:
    Context() = delete;
    explicit Context(std::nullptr_t) { }
    explicit Context(const IWindow* window, const IOSAdapter* osAdapter);
    Context(Context&) = delete;
    Context(Context&&) = default;
    Context& operator=(Context&) = delete;
    Context& operator=(Context&&) = default;
    ~Context() = default;

    [[nodiscard]] const IWindow* Window() const noexcept { return m_window; }
    [[nodiscard]] const Instance& GetInstance() const noexcept { return m_instance; }
    [[nodiscard]] const Surface& GetSurface() const noexcept { return m_surface; }
    [[nodiscard]] const Device& GetDevice() const noexcept { return m_device; }
    [[nodiscard]] const Allocator& GetAllocator() const noexcept { return m_allocator; }

    void TryRepairSurface();

private:
    const IWindow* m_window = nullptr;
    const IOSAdapter* m_osAdapter = nullptr;
    Instance m_instance { nullptr };
    Surface m_surface { nullptr };
    Device m_device { nullptr };
    Allocator m_allocator { nullptr };
};
}
