#pragma once
#include "skylabs/base/sdl/mixer/audio.hpp"

namespace sk::sdl::mixer {
class Track {
public:
    explicit Track(std::nullptr_t) { }
    explicit Track(Mixer& mixer);
    Track(const Track&) = delete;
    Track(Track&&) noexcept;
    Track& operator=(const Track&) = delete;
    Track& operator=(Track&&) noexcept;
    ~Track();

    [[nodiscard]] MIX_Track* operator*() { return m_track; }

    bool SetAudio(Audio& audio) const;

private:
    void Cleanup();

    MIX_Track* m_track = nullptr;
};
}
