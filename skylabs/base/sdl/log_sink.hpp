#pragma once
#include "skylabs/base/logging.hpp"

namespace sk::sdl {
class SK_BASE_PUBLIC_CLASS LogSink final : public log::ISink {
public:
    ~LogSink() override = default;

    void Write(log::Category category, log::Level level, const std::source_location& loc,
               const std::string& message) override;
};
}
