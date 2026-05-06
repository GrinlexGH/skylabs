#include <skylabs/public/sdl/mixer/audio.hpp>
#include <skylabs/public/sdl/filesystem.hpp>

namespace SDL::Mixer {
CAudio::CAudio(CMixer& mixer, const std::string_view uri) {
    std::unique_ptr<IFileStream> stream = Filesystem::LoadAsIO(uri);
    SDL_IOStream* sdlStream = CreateIOStreamFromResource(stream.get());
    m_audio = MIX_LoadAudio_IO(*mixer, sdlStream, false, false);
    if (!m_audio) {
        throw std::runtime_error(SDL_GetError());
    }
}

CAudio::CAudio(CAudio&& other) noexcept :
    m_audio(std::exchange(other.m_audio, nullptr))
{}

CAudio& CAudio::operator=(CAudio&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_audio = std::exchange(other.m_audio, nullptr);
    }
    return *this;
}

CAudio::~CAudio() {
    Cleanup();
}

void CAudio::Cleanup() {
    if (m_audio) {
        MIX_DestroyAudio(m_audio);
        m_audio = nullptr;
    }
}
}
