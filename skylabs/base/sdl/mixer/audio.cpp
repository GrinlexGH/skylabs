#include "skylabs/base/sdl/mixer/audio.hpp"
#include "skylabs/base/sdl/filesystem.hpp"

namespace sk::sdl::mixer {
Audio::Audio(const filesystem::Filesystem& filesystem, Mixer& mixer, const std::string_view uri) {
    const std::unique_ptr<filesystem::IFileStream> stream = filesystem.LoadAsIO(uri);
    SDL_IOStream* sdlStream = FileStream::CreateIOStreamFromResource(stream.get());
    m_audio = MIX_LoadAudio_IO(*mixer, sdlStream, false, false);
}

Audio::Audio(Audio&& other) noexcept : m_audio(std::exchange(other.m_audio, nullptr)) { }

Audio& Audio::operator=(Audio&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_audio = std::exchange(other.m_audio, nullptr);
    }
    return *this;
}

Audio::~Audio() { Cleanup(); }

void Audio::Cleanup() {
    if (m_audio) {
        MIX_DestroyAudio(m_audio);
        m_audio = nullptr;
    }
}
}
