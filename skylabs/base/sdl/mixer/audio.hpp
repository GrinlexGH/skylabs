#pragma once
#include "../../../engine/filesystem.hpp"
#include "skylabs/base/sdl/mixer/mixer.hpp"

namespace sk::sdl::mixer {
class Audio {
public:
    explicit Audio(std::nullptr_t) { }
    explicit Audio(const filesystem::Filesystem& filesystem, Mixer& mixer, std::string_view uri);
    Audio(const Audio&) = delete;
    Audio(Audio&&) noexcept;
    Audio& operator=(const Audio&) = delete;
    Audio& operator=(Audio&&) noexcept;
    ~Audio();

    [[nodiscard]] MIX_Audio* operator*() { return m_audio; }
    operator bool() const { return !!m_audio; }

private:
    void Cleanup();

    MIX_Audio* m_audio = nullptr;
};
}
