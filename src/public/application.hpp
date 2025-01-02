#pragma once

#include "publicapi.hpp"

class PLATFORM_CLASS IApplication
{
public:
    IApplication() = default;
    IApplication(const IApplication&) = default;
    IApplication(IApplication&&) = default;
    IApplication& operator=(const IApplication&) = default;
    IApplication& operator=(IApplication&&) = default;
    virtual ~IApplication() = default;

    virtual void Create() = 0;
    virtual void Main() = 0;
    virtual void Destroy() = 0;
};

class PLATFORM_CLASS CBaseApplication : public IApplication
{
public:
    virtual void PreCreate() {}
    void Create() override {}
    virtual void PostCreate() {}

    virtual void Run();
    void Main() override {}

    virtual void PreDestroy() {}
    void Destroy() override {}
    virtual void PostDestroy() {}
};
