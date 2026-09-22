#include <stdexcept>
#include <utility>

#include "skylabs/base/sdl/mixer/mixer.hpp"

namespace sk::sdl::mixer {
Mixer::Mixer() : m_mixer(MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr)) {
    if (!m_mixer) {
        throw std::runtime_error(SDL_GetError());
    }
}

Mixer::Mixer(Mixer&& other) noexcept : m_mixer(std::exchange(other.m_mixer, nullptr)) { }

Mixer& Mixer::operator=(Mixer&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_mixer = std::exchange(other.m_mixer, nullptr);
    }
    return *this;
}

Mixer::~Mixer() { Cleanup(); }

void Mixer::Cleanup() {
    if (m_mixer) {
        MIX_DestroyMixer(m_mixer);
        m_mixer = nullptr;
    }
}
}
