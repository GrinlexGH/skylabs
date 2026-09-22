#pragma once
#include <cstddef>

#include <SDL3_mixer/SDL_mixer.h>

namespace sk::sdl::mixer {
class Mixer {
public:
    explicit Mixer(std::nullptr_t) { }
    explicit Mixer();
    Mixer(const Mixer&) = delete;
    Mixer(Mixer&&) noexcept;
    Mixer& operator=(const Mixer&) = delete;
    Mixer& operator=(Mixer&&) noexcept;
    ~Mixer();

    [[nodiscard]] MIX_Mixer* operator*() { return m_mixer; }

private:
    void Cleanup();

    MIX_Mixer* m_mixer = nullptr;
};
}
