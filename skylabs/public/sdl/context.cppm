module;
#include <skylabs/public/dll_export.hpp>
#include <SDL3/SDL.h>
export module skylabs.pub.sdl:context;
export import std;

export namespace SDL {
class PUBLIC_CLASS CContext {
public:
    CContext() = delete;
    explicit CContext(std::nullptr_t) {}
    explicit CContext(SDL_InitFlags flags);
    CContext(const CContext&) = delete;
    CContext(CContext&& other) noexcept;
    CContext& operator=(const CContext&) = delete;
    CContext& operator=(CContext&& other) noexcept;
    ~CContext();

private:
    void Cleanup();

    SDL_InitFlags m_flags = 0;
};
}
