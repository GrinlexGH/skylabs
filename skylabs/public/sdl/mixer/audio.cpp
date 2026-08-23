#include <skylabs/public/sdl/mixer/audio.hpp>
#include <skylabs/public/sdl/filesystem.hpp>

namespace SDL::Mixer {
CAudio::CAudio(const CFilesystem& filesystem, CMixer& mixer, const std::string_view uri) {
    std::unique_ptr<IFileStream> stream = filesystem.LoadAsIO(uri);
    SDL_IOStream* sdlStream = CreateIOStreamFromResource(stream.get());
    m_audio = MIX_LoadAudio_IO(*mixer, sdlStream, false, false);
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
