#include "skylabs/base/sdl/mixer/track.hpp"

namespace sk::sdl::mixer {
Track::Track(Mixer& mixer) : m_track(MIX_CreateTrack(*mixer)) {
    if (!m_track) {
        throw std::runtime_error(SDL_GetError());
    }
}

Track::Track(Track&& other) noexcept : m_track(std::exchange(other.m_track, nullptr)) { }

Track& Track::operator=(Track&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_track = std::exchange(other.m_track, nullptr);
    }
    return *this;
}

Track::~Track() { Cleanup(); }

void Track::Cleanup() {
    if (m_track) {
        MIX_DestroyTrack(m_track);
        m_track = nullptr;
    }
}

bool Track::SetAudio(Audio& audio) const { return MIX_SetTrackAudio(m_track, *audio); }
}
