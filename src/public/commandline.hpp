#pragma once

#include "platform.hpp"
#include <string>
#include <vector>

class ICommandLine {
public:
  virtual void CreateCmdLine(int argc,
                             const std::vector<std::string> &argv) = 0;
  virtual int CheckParm(std::string_view parm) = 0;
};

PLATFORM_INTERFACE ICommandLine *CommandLine();
