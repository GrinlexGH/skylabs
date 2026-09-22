#pragma once
#include <SDL3/SDL_events.h>

#include "skylabs/base/event_pump.hpp"

namespace sk::sdl {
class SK_BASE_PUBLIC_CLASS EventPump final : public input::IEventPump {
public:
    void SetEventFilter(input::EventFilter filter, void* userData) override;
    [[nodiscard]] std::optional<input::Event> PollEvent() override;

private:
    static input::Event TranslateEvent(const SDL_Event& event);
    static bool EventFilterWrap(void* userData, SDL_Event* event);

    void* m_filterUserData = nullptr;
    input::EventFilter m_filter = nullptr;
};
}
