#pragma once

#include "platform.hpp"

class IApplication {
public:
    virtual void Create() = 0;
    virtual void Main() = 0;
    virtual void Destroy() = 0;
};

class PLATFORM_CLASS CBaseApplication : public IApplication {
public:
    virtual void PreCreate()    { }
    virtual void Create()       { }
    virtual void PostCreate()   { }

    virtual void Run();
    virtual void Main()         { }

    virtual void PreDestroy()   { }
    virtual void Destroy()      { }
    virtual void PostDestroy()  { }
};
