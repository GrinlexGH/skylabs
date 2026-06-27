#pragma once
#include <SDL3_mixer/SDL_mixer.h>
import std;

namespace SDL::Mixer {
class CMixer
{
public:
    explicit CMixer(std::nullptr_t) {}
    explicit CMixer();
    CMixer(const CMixer&) = delete;
    CMixer(CMixer&&) noexcept;
    CMixer& operator=(const CMixer&) = delete;
    CMixer& operator=(CMixer&&) noexcept;
    ~CMixer();

    [[nodiscard]] MIX_Mixer* operator*() { return m_mixer; }

private:
    void Cleanup();

    MIX_Mixer* m_mixer = nullptr;
};
}
