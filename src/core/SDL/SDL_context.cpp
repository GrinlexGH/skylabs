#include "SDL_context.hpp"

#include <SDL3/SDL.h>
#include "app_metadata.hpp"
#include "logging.hpp"

#include <stdexcept>
#include <format>

namespace SDL {
int CGlobalContext::m_refCount = 0;

CGlobalContext::CGlobalContext() {
    if (!m_refCount) {
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, App::Metadata::NAME);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, App::Metadata::VERSION);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, App::Metadata::COMPANY);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, App::Metadata::COPYRIGHT);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, App::Metadata::HOMEPAGE);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");

        if (!SDL_Init(0)) {
            SDL_Quit();
            throw std::runtime_error(std::format("Failed to initialize SDL: {}!", SDL_GetError()));
        }
    } else {
        Log::Warning("SDL already initialized {} time(s).", m_refCount);
    }

    ++m_refCount;
}

CGlobalContext::~CGlobalContext() {
    if (!--m_refCount) {
        SDL_Quit();
    }
}

CSubSystemContext::CSubSystemContext(const SDL_InitFlags flags) : m_subSystems(flags) {
    if (!SDL_InitSubSystem(flags)) {
        throw std::runtime_error(std::format("Failed to initialize SDL: {}!", SDL_GetError()));
    }
}

CSubSystemContext::~CSubSystemContext() {
    SDL_QuitSubSystem(m_subSystems);
}
}
