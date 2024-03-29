#ifdef _WIN32
#include <Windows.h>
#endif
#include <string>
#include <vector>
#include <algorithm>
#include "icommandline.hpp"

class CCommandLine : public ICommandLine {
public:
    virtual void CreateCmdLine(const int& argc, const std::vector<std::string>& argv) override final;
    virtual int CheckParm(std::string_view parm) override final;

    friend ICommandLine& CommandLine();

protected:
    // singleton stuff
    CCommandLine() = default;
    ~CCommandLine() = default;
    CCommandLine(const CCommandLine&) = delete;
    CCommandLine(CCommandLine&&) = delete;
    CCommandLine& operator=(const CCommandLine&) = delete;
    CCommandLine& operator=(CCommandLine&&) = delete;

private:
    int argc_;
    std::vector<std::string> argv_;
};

ICommandLine& CommandLine() {
    static CCommandLine instance;
    return instance;
}

void CCommandLine::CreateCmdLine(const int& argc, const std::vector<std::string>& argv) {
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

