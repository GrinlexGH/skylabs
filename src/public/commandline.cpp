#include <algorithm>
#include <string>
#include <vector>

#include "console.hpp"
#include "commandline.hpp"

class CCommandLine final : public ICommandLine
{
public:
    void CreateCmdLine(const std::vector<std::string> &argv) override;
    int FindParam(std::string_view parm) override;
};

static CCommandLine g_CmdLine;
PLATFORM_INTERFACE ICommandLine *CommandLine() { return &g_CmdLine; }

void CCommandLine::CreateCmdLine(const std::vector<std::string> &argv) {
    argv_ = argv;
}

// Returns index of found parameter. 0 if not found.
int CCommandLine::FindParam(std::string_view parm) {
    auto it = std::find((argv_.begin()) + 1, argv_.end(), parm);
    if (it == std::end(argv_))
        return 0;
    return static_cast<int>(std::distance(argv_.begin(), it));
}
