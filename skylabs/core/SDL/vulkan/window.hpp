#pragma once
#include <skylabs/core/render/vulkan/platform/window.hpp>

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

    [[nodiscard]] SDL_Window* operator*() const noexcept { return m_handle; }
    [[nodiscard]] SDL_Window* Handle() const noexcept { return m_handle; }

    [[nodiscard]] Utils::Extent2D DrawableSize() const override;
    [[nodiscard]] std::span<const char* const> GetRequiredInstanceExtensions() const override;
    [[nodiscard]] vk::SurfaceKHR CreateSurface(const vk::Instance& instance) const override;
    void DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const override;

    [[nodiscard]] bool IsQueueFamilySupportPresent(
        const vk::Instance& instance,
        const vk::PhysicalDevice& physicalDevice,
        std::uint32_t index
    ) const override;

private:
    SDL_Window* m_handle = nullptr;
};
}
