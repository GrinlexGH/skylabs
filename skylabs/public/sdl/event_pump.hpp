#pragma once
#include <skylabs/public/event_pump.hpp>

namespace SDL {
class PUBLIC_CLASS CEventPump final : public IEventPump
{
public:
    void SetEventFilter(EventFilter filter, void* userData) override;
    [[nodiscard]] std::optional<Event> PollEvent() override;

private:
    static Event TranslateEvent(const SDL_Event& event);
    static bool EventFilterWrap(void* userData, SDL_Event* event);

    void* m_filterUserData = nullptr;
    EventFilter m_filter = nullptr;
};
}
