#pragma once
#include <skylabs/core/render/vulkan/window.hpp>

#include <SDL3/SDL.h>

namespace SDL::Vulkan {
class CWindow final : public ::Vulkan::IWindow
{
public:
    explicit CWindow(const char* title, int w, int h, SDL_WindowFlags flags = 0);
    CWindow(const CWindow&) = delete;
    CWindow(CWindow&& other) noexcept;
    CWindow& operator=(const CWindow&) = delete;
    CWindow& operator=(CWindow&& rhs) noexcept;
    ~CWindow() override;

    [[nodiscard]] auto operator*() const noexcept -> SDL_Window* { return m_handle; }
    [[nodiscard]] auto GetHandle() const noexcept -> SDL_Window* { return m_handle; }

    auto GetDrawableSize(int& w, int& h) const -> void override;

    [[nodiscard]] auto GetRequiredInstanceExtensions() const -> std::span<const char* const> override;

    [[nodiscard]] auto CreateSurface(const vk::Instance& instance) const -> vk::SurfaceKHR override;
    auto DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const -> void override;

    [[nodiscard]] auto IsQueueFamilyPresentSupport(
        const vk::Instance& instance,
        const vk::PhysicalDevice& physicalDevice,
        std::uint32_t index
    ) const -> bool override;

private:
    SDL_Window* m_handle = nullptr;
};
}
