module;
#include <skylabs/public/dll_export.hpp>
#include <SDL3/SDL.h>
export module skylabs.pub.sdl:window;
export import skylabs.pub.window;

export namespace SDL {
class PUBLIC_CLASS CWindow final : public IWindow
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

    [[nodiscard]] Utils::Extent2D DrawableSize() const override;

    // Vulkan
    [[nodiscard]] vk::SurfaceKHR CreateSurface(vk::Instance instance) const override;

private:
    SDL_Window* m_handle = nullptr;
};
}
