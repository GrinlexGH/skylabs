#ifdef _WIN32
#include <Windows.h>
#endif
#include <string>
#include <vector>
#include <algorithm>
#include "icommandline.hpp"

class CCommandLine : public ICommandLine {
public:
    CCommandLine() : argc_(1), argv_(argc_) {}
    virtual ~CCommandLine() final;

    virtual void CreateCmdLine(int& argc, const std::vector<std::string>& argv) override final;
    virtual int CheckParm(std::string_view parm) override final;

    friend ICommandLine* CommandLine();

    // singleton stuff
    CCommandLine(CCommandLine&) = delete;
    CCommandLine(CCommandLine&&) = delete;
    void operator=(const CCommandLine&) = delete;
    CCommandLine& operator=(CCommandLine&&) = delete;
private:
    int argc_;
    std::vector<std::string> argv_;

    static CCommandLine* instance_;
};

CCommandLine* CCommandLine::instance_ = new CCommandLine();
ICommandLine* CommandLine()
{
    return CCommandLine::instance_;
}

CCommandLine::~CCommandLine() {
    delete instance_;
}

void CCommandLine::CreateCmdLine(int& argc, const std::vector<std::string>& argv) {
    argc_ = argc;
    argv_ = argv;
}

int CCommandLine::CheckParm(std::string_view parm) {
    
    auto it = std::find(argv_.begin() + 1, argv_.end(), parm);
    if (it == std::end(argv_)) {
        return 0;
    }
    return (int)std::distance(argv_.begin(), it);
}

