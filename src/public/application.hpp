#pragma once

#include <filesystem>
#include <string>

class IApplication
{
public:
    virtual void Init() = 0;
    virtual void SwitchDebugMode() { debugMode_ = !debugMode_; }
    virtual bool GetDebugMode() { return debugMode_; }
    virtual std::filesystem::path GetRootDir() { return rootDir_; }

protected:
    // Singleton stuff
    IApplication() = default;
    IApplication(const IApplication &) = delete;
    IApplication(IApplication &&) = delete;
    IApplication &operator=(const IApplication &) = delete;
    IApplication &operator=(IApplication &&) = delete;

    bool debugMode_;
    std::filesystem::path rootDir_;
};
