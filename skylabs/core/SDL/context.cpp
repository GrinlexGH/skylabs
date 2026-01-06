#include <skylabs/core/SDL/context.hpp>
#include <skylabs/public/logging.hpp>
#include "project_info.hpp"

#include <fmt/ranges.h>

#include <array>
#include <cassert>
#include <mutex>
#include <stdexcept>

namespace {
std::string ToString(const SDL_InitFlags flags) {
    std::array<std::string_view, 8> active_flags {};
    size_t count = 0;

    if (flags & SDL_INIT_AUDIO) active_flags[count++] = "AUDIO";
    if (flags & SDL_INIT_VIDEO) active_flags[count++] = "VIDEO";
    if (flags & SDL_INIT_JOYSTICK) active_flags[count++] = "JOYSTICK";
    if (flags & SDL_INIT_HAPTIC) active_flags[count++] = "HAPTIC";
    if (flags & SDL_INIT_GAMEPAD) active_flags[count++] = "GAMEPAD";
    if (flags & SDL_INIT_EVENTS) active_flags[count++] = "EVENTS";
    if (flags & SDL_INIT_SENSOR) active_flags[count++] = "SENSOR";
    if (flags & SDL_INIT_CAMERA) active_flags[count++] = "CAMERA";

    return fmt::format("{}", fmt::join(active_flags.begin(), active_flags.begin() + count, " | "));
}
}

namespace SDL {
CContext::CContext(const SDL_InitFlags flags) {
    static std::once_flag metaFlag;
    std::call_once(metaFlag, [] {
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, Skylabs::NAME);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, Skylabs::VERSION);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, Skylabs::COMPANY);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, Skylabs::COPYRIGHT);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, Skylabs::HOMEPAGE);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");
    });

    Log::Debug("Initializing SDL subsystems: {}...", ToString(flags));
    if (!SDL_InitSubSystem(flags)) {
        throw std::runtime_error(SDL_GetError());
    }

    m_flags = flags;
}

CContext::CContext(CContext&& other) noexcept : m_flags(std::exchange(other.m_flags, 0))
{}

CContext& CContext::operator=(CContext&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_flags = std::exchange(other.m_flags, 0);
    }
    return *this;
}

CContext::~CContext() {
    Cleanup();
}

void CContext::Cleanup() {
    if (m_flags != 0) {
        Log::Debug("Quiting from SDL subsystems: {}", ToString(m_flags));
        SDL_QuitSubSystem(m_flags);
        m_flags = 0;
    }
}
}
