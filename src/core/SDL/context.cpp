#include "context.hpp"

#include "project_info.hpp"
#include "logging.hpp"

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

        if (!SDL_InitSubSystem(flags)) {
            CContext::m_refCount = 0;
            SDL_Quit();
            throw std::runtime_error(std::format("Failed to initialize SDL: {}!", SDL_GetError()));
        }
    } else {
        Log::Warning("SDL already initialized {} time(s). Specified subsystems have not been initialized.", m_refCount);
    }

    ++m_refCount;
}

CContext::~CContext() {
    if (!--m_refCount) {
        SDL_Quit();
    }
}
}
