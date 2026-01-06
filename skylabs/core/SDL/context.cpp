#include <skylabs/core/SDL/context.hpp>
#include <skylabs/public/logging.hpp>
#include "project_info.hpp"

#include <fmt/format.h>

namespace SDL {
int CContext::m_refCount = 0;

CContext::CContext(const SDL_InitFlags flags) {
    if (!m_refCount) {
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, Skylabs::NAME);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, Skylabs::VERSION);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, Skylabs::COMPANY);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, Skylabs::COPYRIGHT);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, Skylabs::HOMEPAGE);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");
    }

    if (!SDL_InitSubSystem(flags)) {
        m_refCount = 0;
        SDL_Quit();
        throw std::runtime_error(fmt::format("Failed to initialize SDL: {}!", SDL_GetError()));
    }

    ++m_refCount;
}

CContext::~CContext() {
    if (!--m_refCount) {
        Log::Debug("Quiting SDL...");
        SDL_Quit();
    }
}
}
