module;
#include <skylabs/public/dll_export.hpp>
export module skylabs.pub.window;
export import skylabs.pub.utils;
export import vulkan;

export class PUBLIC_CLASS IWindow
{
public:
    IWindow() = default;
    IWindow(const IWindow&) = delete;
    IWindow(IWindow&&) noexcept = default;
    IWindow& operator=(const IWindow&) = delete;
    IWindow& operator=(IWindow&&) noexcept = default;
    virtual ~IWindow() = default;

    [[nodiscard]] virtual Utils::Extent2D DrawableSize() const = 0;

    // Vulkan
    [[nodiscard]] virtual vk::SurfaceKHR CreateSurface(vk::Instance instance) const = 0;
};
