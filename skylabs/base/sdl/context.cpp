#include <mutex>
#include <stdexcept>

#include <SDL3/SDL.h>

#include "project_info.hpp"
#include "skylabs/base/sdl/context.hpp"

namespace sk::sdl {
Context::Context(const SDL_InitFlags flags) {
    static std::once_flag metaFlag;
    std::call_once(metaFlag, [] {
        SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_TRACE);

        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, Skylabs::NAME);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, Skylabs::VERSION);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, Skylabs::COMPANY);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, Skylabs::COPYRIGHT);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, Skylabs::HOMEPAGE);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");

        SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft");
    });

    if (!SDL_InitSubSystem(flags)) {
        throw std::runtime_error(SDL_GetError());
    }

    m_flags = flags;
}

Context::Context(Context&& other) noexcept : m_flags(std::exchange(other.m_flags, 0)) { }

Context& Context::operator=(Context&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_flags = std::exchange(other.m_flags, 0);
    }
    return *this;
}

Context::~Context() { Cleanup(); }

void Context::Cleanup() {
    if (m_flags) {
        SDL_QuitSubSystem(m_flags);
        m_flags = 0;
    }
}
}
