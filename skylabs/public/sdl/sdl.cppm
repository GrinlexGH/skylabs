module;
#include <SDL3/SDL.h>
#include <skylabs/public/dll_export.hpp>
export module skylabs.pub.sdl;
export import :context;
export import :window;
export import :filesystem;

export import skylabs.pub.utils;
export import vulkan;

export namespace SDL {
PUBLIC_CLASS std::span<const bool> GetKeyboardState();
PUBLIC_CLASS Utils::Extent2D GetWindowSizeInPixels(SDL_Window* window);

namespace Vulkan {
    PUBLIC_CLASS vk::SurfaceKHR CreateSurface(SDL_Window* window, const vk::Instance& instance);
}
}
