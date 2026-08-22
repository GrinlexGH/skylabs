#pragma once
#include <skylabs/public/logging.hpp>

namespace SDL {
class PUBLIC_CLASS CLogSink final : public Log::ISink
{
public:
    ~CLogSink() override = default;
    void Write(
        Log::Category category,
        Log::Level level,
        const std::source_location& loc,
        const std::string& message
    ) override;
};
}
