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
  int m_argc = 0;
  std::vector<std::string> m_argv;
};

static CCommandLine instance;
ICommandLine *CommandLine() { return &instance; }

// Should call only once in one at start of program
void CCommandLine::CreateCmdLine(int argc,
                                 const std::vector<std::string> &argv) {
  m_argc = argc;
  m_argv = argv;
}

int CCommandLine::CheckParm(std::string_view parm) {
  auto it = std::find(m_argv.begin() + 1, m_argv.end(), parm);
  if (it == std::end(m_argv)) {
    return 0;
  }
  return static_cast<int>(std::distance(m_argv.begin(), it));
}
