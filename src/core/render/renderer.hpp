#pragma once
#include <stdexcept>

class IRenderer
{
public:
    IRenderer() = default;
    IRenderer(const IRenderer&) = delete;
    IRenderer(IRenderer&&) = default;
    IRenderer& operator=(const IRenderer&) = delete;
    IRenderer& operator=(IRenderer&&) = default;
    virtual ~IRenderer() = default;

    virtual void Draw() = 0;
};

class CRendererError final : public std::runtime_error
{
public:
    explicit CRendererError(const std::string& message) : std::runtime_error(message.c_str()) {}
    explicit CRendererError(const char* message) : std::runtime_error(message) {}
};

