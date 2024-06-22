#include "commandline.hpp"
#include <algorithm>
#include <string>
#include <vector>

class CCommandLine : public ICommandLine {
public:
  CCommandLine() = default;
  ~CCommandLine() = default;

  void CreateCmdLine(int argc,
                     const std::vector<std::string> &argv) override final;
  int CheckParm(std::string_view parm) override final;

protected:
  // Singleton stuff
  CCommandLine(const CCommandLine &) = delete;
  CCommandLine(CCommandLine &&) = delete;
  CCommandLine &operator=(const CCommandLine &) = delete;
  CCommandLine &operator=(CCommandLine &&) = delete;

private:
  int argc_ = 0;
  std::vector<std::string> argv_;
};

static CCommandLine g_CmdLine;
PLATFORM_INTERFACE ICommandLine *CommandLine() { return &g_CmdLine; }

// Should call only once in one at start of program
void CCommandLine::CreateCmdLine(int argc,
                                 const std::vector<std::string> &argv) {
  argc_ = argc;
  argv_ = argv;
}

int CCommandLine::CheckParm(std::string_view parm) {
  auto it = std::find(argv_.begin() + 1, argv_.end(), parm);
  if (it == std::end(argv_)) {
    return 0;
  }
  return static_cast<int>(std::distance(argv_.begin(), it));
}
