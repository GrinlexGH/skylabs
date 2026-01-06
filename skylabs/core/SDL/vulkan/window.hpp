#pragma once
#include <skylabs/core/render/vulkan/window.hpp>

#include <SDL3/SDL.h>

namespace SDL::Vulkan {
class CWindow final : public ::Vulkan::IWindow
{
public:
    CWindow() = delete;
    explicit CWindow(std::nullptr_t) noexcept {}
    explicit CWindow(const char* title, int w, int h, SDL_WindowFlags flags = 0);
    CWindow(const CWindow&) = delete;
    CWindow(CWindow&& other) noexcept;
    CWindow& operator=(const CWindow&) = delete;
    CWindow& operator=(CWindow&& rhs) noexcept;
    ~CWindow() override;

    [[nodiscard]] auto operator*() const noexcept -> SDL_Window* { return m_handle; }
    [[nodiscard]] auto Handle() const noexcept -> SDL_Window* { return m_handle; }

    auto DrawableSize() const -> CExtent2D override;

    [[nodiscard]] auto GetRequiredInstanceExtensions() const -> std::span<const char* const> override;

    [[nodiscard]] auto CreateSurface(const vk::Instance& instance) const -> vk::SurfaceKHR override;
    auto DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const -> void override;

    [[nodiscard]] auto IsQueueFamilySupportPresent(
        const vk::Instance& instance,
        const vk::PhysicalDevice& physicalDevice,
        std::uint32_t index
    ) const -> bool override;

private:
    SDL_Window* m_handle = nullptr;
};
}
