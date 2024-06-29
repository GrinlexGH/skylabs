#pragma once

#include "platform.hpp"
#include <string>
#include <vector>

class ICommandLine
{
public:
    virtual void CreateCmdLine(const std::vector<std::string> &argv) = 0;
    virtual int FindParam(std::string_view parm) = 0;

protected:
    std::vector<std::string> argv_;
};

PLATFORM_INTERFACE ICommandLine *CommandLine();
