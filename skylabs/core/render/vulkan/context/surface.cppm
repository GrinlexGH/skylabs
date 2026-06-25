module;
#include <skylabs/public/window.hpp>

export module skylabs.vulkan.context:surface;
export import :instance;

export namespace Vulkan {
class CSurface
{
public:
    explicit CSurface(std::nullptr_t) {}
    explicit CSurface(const CInstance& instance, const IWindow* window) :
        m_handle(*instance, window->CreateSurface(*instance)) {}
    CSurface(const CSurface&) = delete;
    CSurface(CSurface&& other) noexcept = default;
    CSurface& operator=(const CSurface&) = delete;
    CSurface& operator=(CSurface&& rhs) noexcept = default;
    ~CSurface() = default;

    [[nodiscard]] const vk::raii::SurfaceKHR& operator*() const noexcept { return m_handle; }

private:
    vk::raii::SurfaceKHR m_handle { nullptr };
};
}
