#pragma once

class IRendererContext
{
public:
    virtual ~IRendererContext() = default;

    virtual void Initialize() = 0;
};
