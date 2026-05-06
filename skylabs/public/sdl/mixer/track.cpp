#include <skylabs/public/sdl/mixer/track.hpp>

namespace SDL::Mixer {
CTrack::CTrack(CMixer& mixer) : m_track(MIX_CreateTrack(*mixer)) {
    if (!m_track) {
        throw std::runtime_error(SDL_GetError());
    }
}

CTrack::CTrack(CTrack&& other) noexcept :
    m_track(std::exchange(other.m_track, nullptr))
{}

CTrack& CTrack::operator=(CTrack&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_track = std::exchange(other.m_track, nullptr);
    }
    return *this;
}

CTrack::~CTrack() {
    Cleanup();
}

void CTrack::Cleanup() {
    if (m_track) {
        MIX_DestroyTrack(m_track);
        m_track = nullptr;
    }
}

bool CTrack::SetAudio(CAudio& audio) const {
    return MIX_SetTrackAudio(m_track, *audio);
}
}
