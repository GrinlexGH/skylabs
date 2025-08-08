#pragma once
#include "../../window.hpp"

#include <SDL3/SDL.h>

namespace SDL::Vulkan {
class CWindow : public IWindow
{
public:
    explicit CWindow(const char* title, int w, int h, SDL_WindowFlags flags = 0);
    CWindow(const CWindow&) = delete;
    CWindow(CWindow&& other) noexcept;
    CWindow& operator=(const CWindow&) = delete;
    CWindow& operator=(CWindow&& rhs) noexcept;
    ~CWindow() override;

    [[nodiscard]] SDL_Window* GetHandle() const { return m_handle; }

    // [[nodiscard]] std::vector<const char*> GetRequiredInstanceExtensions() const override;
    // [[nodiscard]] bool CheckQueuePresentSupport(
    //     const vk::Instance& instance,
    //     const vk::PhysicalDevice& physicalDevice,
    //     std::uint32_t queueFamilyIndex
    // ) const override;
    //
    // [[nodiscard]] vk::SurfaceKHR CreateSurface(const vk::Instance& instance) const override;
    // void DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const override;

    void GetDrawableSize(int* w, int* h) const override;

private:
    SDL_Window* m_handle = nullptr;
};
}
