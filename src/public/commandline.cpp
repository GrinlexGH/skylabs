#ifdef _WIN32
#include <Windows.h>
#endif
#include "commandline.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

class CCommandLine : public ICommandLine {
public:
  CCommandLine() = default;
  ~CCommandLine() = default;

  virtual void
  CreateCmdLine(int argc, const std::vector<std::string> &argv) override final;
  virtual int CheckParm(std::string_view parm) override final;

  friend ICommandLine *CommandLine();

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

static CCommandLine instance;
ICommandLine *CommandLine() { return &instance; }

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
