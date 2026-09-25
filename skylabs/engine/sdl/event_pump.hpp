#pragma once
#include <SDL3/SDL.h>

#include "skylabs/engine/event_pump.hpp"

namespace sk::sdl {
class EventPump final : public input::IEventPump {
public:
    void SetEventFilter(input::EventFilter filter, void* userData) override;
    [[nodiscard]] std::optional<input::Event> PollEvent() override;

private:
    static bool EventFilterWrap(void* userData, SDL_Event* event);

    void* m_filterUserData = nullptr;
    input::EventFilter m_filter = nullptr;
};
}
