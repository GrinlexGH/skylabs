#include <skylabs/public/sdl/mixer/mixer.hpp>

namespace SDL::Mixer {
CMixer::CMixer() : m_mixer(MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr)) {
    if (!m_mixer) {
        throw std::runtime_error(SDL_GetError());
    }
}

CMixer::CMixer(CMixer&& other) noexcept :
    m_mixer(std::exchange(other.m_mixer, nullptr))
{}

CMixer& CMixer::operator=(CMixer&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_mixer = std::exchange(other.m_mixer, nullptr);
    }
    return *this;
}

CMixer::~CMixer() {
    Cleanup();
}

void CMixer::Cleanup() {
    if (m_mixer) {
        MIX_DestroyMixer(m_mixer);
        m_mixer = nullptr;
    }
}
}
