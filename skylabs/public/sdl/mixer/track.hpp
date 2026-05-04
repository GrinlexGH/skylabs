#pragma once
#include <skylabs/public/pch.hpp>
#include <skylabs/public/sdl/mixer/mixer.hpp>

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

private:
    void Cleanup();

    MIX_Track* m_track = nullptr;
};
}
