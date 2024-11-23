#include "commandline.hpp"

#include <algorithm>
#include <string>
#include <vector>

class CCommandLine final : public ICommandLine {
public:
    void CreateCmdLine(const std::vector<std::string>& argv) override;
    int FindParam(std::string_view param) override;
};

static CCommandLine g_cmdLine;
PLATFORM_INTERFACE ICommandLine* CommandLine() { return &g_cmdLine; }

void CCommandLine::CreateCmdLine(const std::vector<std::string>& argv) {
    m_argv = argv;
}

int CCommandLine::FindParam(std::string_view param) {
    auto it = std::find(m_argv.begin() + 1, m_argv.end(), param);
    if (it == m_argv.end()) {
        return 0;
    }
    return static_cast<int>(std::distance(m_argv.begin(), it));
}
