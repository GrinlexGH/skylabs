#pragma once
#include <skylabs/public/application.hpp>
#include <skylabs/core/SDL/context.hpp>
#include <skylabs/core/SDL/vulkan/window.hpp>

// Fix defines
#ifdef CreateWindow
#undef CreateWindow
#endif

namespace Vulkan {
class CVulkanMono : CBaseApplication
{
public:
    void Run() override;

private:
    void CreateWindow();

    SDL::CContext m_SDLContext { nullptr };
    SDL::Vulkan::CWindow m_SDLWindow { nullptr };
};
}
