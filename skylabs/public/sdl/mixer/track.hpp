#pragma once
#include <skylabs/public/sdl/mixer/audio.hpp>

namespace SDL::Mixer {
class CTrack
{
public:
    explicit CTrack(std::nullptr_t) {}
    explicit CTrack(CMixer& mixer);
    CTrack(const CTrack&) = delete;
    CTrack(CTrack&&) noexcept;
    CTrack& operator=(const CTrack&) = delete;
    CTrack& operator=(CTrack&&) noexcept;
    ~CTrack();

    [[nodiscard]] MIX_Track* operator*() { return m_track; }

    bool SetAudio(CAudio& audio) const;

private:
    void Cleanup();

    MIX_Track* m_track = nullptr;
};
}
