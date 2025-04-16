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

class CRendererInitError final : public std::runtime_error
{
public:
    explicit CRendererInitError(const std::string& message) : std::runtime_error(message.c_str()) {}
    explicit CRendererInitError(const char* message) : std::runtime_error(message) {}
};

