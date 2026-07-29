#include <skylabs/public/sdl/event_pump.hpp>

namespace SDL {
std::optional<Event> CEventPump::PollEvent() {
    SDL_Event event;
    SDL_PollEvent(&event);
    switch (event.type) {
        case SDL_EVENT_QUIT: return QuitEvent {};
        default: return std::nullopt;
    }
}
}
