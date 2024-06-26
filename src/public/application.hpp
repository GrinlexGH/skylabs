#pragma once

#include <filesystem>
#include <string>

class IApplication
{
public:
    IApplication() = default;
    virtual int Run() { return 0; };
    virtual void SwitchDebugMode() { debugMode_ = !debugMode_; }
    virtual bool GetDebugMode() { return debugMode_; }
    virtual std::filesystem::path GetRootDir() { return rootDir_; }

protected:
    bool debugMode_ = false;
    std::filesystem::path rootDir_;
};
