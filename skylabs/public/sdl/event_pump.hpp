#pragma once
#include <skylabs/public/event_pump.hpp>

namespace SDL {
class PUBLIC_CLASS CEventPump final : public IEventPump
{
public:
    [[nodiscard]] std::optional<Event> PollEvent() override;
};
}
