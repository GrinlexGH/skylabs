#pragma once
#include <skylabs/public/sdl/mixer/mixer.hpp>

namespace SDL::Mixer {
class CAudio
{
public:
    explicit CAudio(std::nullptr_t) {}
    explicit CAudio(CMixer& mixer, std::string_view uri);
    CAudio(const CAudio&) = delete;
    CAudio(CAudio&&) noexcept;
    CAudio& operator=(const CAudio&) = delete;
    CAudio& operator=(CAudio&&) noexcept;
    ~CAudio();

    [[nodiscard]] MIX_Audio* operator*() { return m_audio; }
    operator bool() const { return !!m_audio; }

private:
    void Cleanup();

    MIX_Audio* m_audio = nullptr;
};
}
