#pragma once

#include "platform.hpp"

class PLATFORM_CLASS IApplication {
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

class PLATFORM_CLASS CBaseApplication : public IApplication {
public:
    CBaseApplication() = default;
    CBaseApplication(const CBaseApplication&) = default;
    CBaseApplication(CBaseApplication&&) = default;
    CBaseApplication& operator=(const CBaseApplication&) = default;
    CBaseApplication& operator=(CBaseApplication&&) = default;
    virtual ~CBaseApplication() = default;

    virtual void PreCreate() { }
    virtual void Create() { }
    virtual void PostCreate() { }

    virtual void Run();
    virtual void Main() { }

    virtual void PreDestroy() { }
    virtual void Destroy() { }
    virtual void PostDestroy() { }
};
