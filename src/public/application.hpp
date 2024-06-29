#pragma once

#include <filesystem>
#include <string>

class IApplication
{
public:
    IApplication() = default;
    virtual ~IApplication() = default;
    virtual int Run() { return 0; }
    virtual void SwitchDebugMode() { debugMode_ = !debugMode_; }
    virtual bool isDebugMode() { return debugMode_; }

protected:
    bool debugMode_ = false;
};
