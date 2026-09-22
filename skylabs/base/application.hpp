#pragma once
#include "sk_base_export.h"

namespace sk {
class SK_BASE_PUBLIC_CLASS IApplication {
public:
    IApplication() = default;
    virtual ~IApplication() = default;

    virtual void Create() = 0;
    virtual void Main() = 0;
    virtual void Destroy() = 0;
};

class SK_BASE_PUBLIC_CLASS BaseApplication : public IApplication {
public:
    virtual void PreCreate() { }
    void Create() override { }
    virtual void PostCreate() { }

    virtual void Run();
    void Main() override { }

    virtual void PreDestroy() { }
    void Destroy() override { }
    virtual void PostDestroy() { }
};
}
