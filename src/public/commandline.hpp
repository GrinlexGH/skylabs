#pragma once

#include "platform.hpp"
#include <string>
#include <vector>

class ICommandLine
{
public:
    virtual void CreateCmdLine(std::vector<std::string_view> &&argv) = 0;
    virtual void CreateCmdLine(const std::vector<std::string_view> &argv) = 0;
    virtual int FindParam(std::string_view parm) = 0;
};

PLATFORM_INTERFACE ICommandLine *CommandLine();
