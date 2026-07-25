#pragma once
#include <skylabs/public/pch.hpp>

struct QuitEvent {};

using Event = std::variant<
    QuitEvent
>;

class IEventPump
{
public:
    virtual ~IEventPump() = default;

    [[nodiscard]] virtual std::optional<Event> PollEvent() = 0;
};
