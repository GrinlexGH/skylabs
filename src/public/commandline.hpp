#pragma once

#include "platform.hpp"

#include <string>
#include <vector>

class ICommandLine {
public:
    virtual ~ICommandLine() = default;

    virtual void CreateCmdLine(const std::vector<std::string>& argv) = 0;
    virtual int FindParam(std::string_view param) = 0;

protected:
    std::vector<std::string> m_argv;
};

PLATFORM_INTERFACE ICommandLine* CommandLine();
