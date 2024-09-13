#include "commandline.hpp"

#include <algorithm>
#include <string>
#include <vector>

class CCommandLine final : public ICommandLine {
public:
    CCommandLine() = default;
    CCommandLine(const CCommandLine&) = default;
    CCommandLine(CCommandLine&&) = default;
    CCommandLine& operator=(const CCommandLine&) = default;
    CCommandLine& operator=(CCommandLine&&) = default;
    ~CCommandLine() = default;

    void CreateCmdLine(const std::vector<std::string>& argv) override;
    int FindParam(std::string_view param) override;
};

static CCommandLine g_cmdLine;
PLATFORM_INTERFACE ICommandLine* CommandLine() { return &g_cmdLine; }

void CCommandLine::CreateCmdLine(const std::vector<std::string>& argv) {
    m_argv = argv;
}

// Returns index of found parameter. 0 if not found.
int CCommandLine::FindParam(std::string_view param) {
    auto it = std::find(m_argv.begin() + 1, m_argv.end(), param);
    if (it == std::end(m_argv)) {
        return 0;
    }
    return static_cast<int>(std::distance(m_argv.begin(), it));
}
