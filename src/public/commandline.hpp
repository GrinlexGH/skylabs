#pragma once

#include "publicapi.hpp"

#include <string>
#include <vector>

class ICommandLine {
public:
    ICommandLine() = default;
    ICommandLine(const ICommandLine&) = default;
    ICommandLine(ICommandLine&&) = default;
    ICommandLine& operator=(const ICommandLine&) = default;
    ICommandLine& operator=(ICommandLine&&) = default;
    virtual ~ICommandLine() = default;

    virtual void CreateCmdLine(const std::vector<std::string>& argv) = 0;

    // Returns index of found parameter. 0 if not found.
    virtual int FindParam(std::string_view param) = 0;

protected:
    std::vector<std::string> m_argv;
};

PLATFORM_INTERFACE ICommandLine* CommandLine();
