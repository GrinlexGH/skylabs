#ifndef ICOMMANDLINE_HPP
#define ICOMMANDLINE_HPP
#ifdef _WIN32
#pragma once
#endif

#include <string>
#include <vector>
#include "platform.hpp"

abstract_class ICommandLine{
public:
    virtual void CreateCmdLine(const int& argc, const std::vector<std::string>& argv) = 0;
    virtual int CheckParm(std::string_view parm) = 0;
};

ICommandLine& CommandLine();

#endif

