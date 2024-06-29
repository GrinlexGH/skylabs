#include <vector>
#include <string>

#include "platform.hpp"
#include "application.hpp"
#include "commandline.hpp"
#include "console.hpp"
#include "unicode.hpp"

class CLauncher final : public IApplication
{
public:
    CLauncher();
    ~CLauncher();
    int Run() override;
    void SwitchDebugMode() override;
};

#ifdef _WIN32
#include <Windows.h>

DLL_EXPORT int CoreInit(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine,
    int nShowCmd)
{
#else
DLL_EXPORT int CoreInit(int argc, char** argv)
{
#endif
    try {
#ifdef _WIN32
        UNUSED(hInstance);
        UNUSED(hPrevInstance);
        UNUSED(lpCmdLine);
        UNUSED(nShowCmd);
        {
            int argc;
            wchar_t** wchar_arg_list{ ::CommandLineToArgvW(::GetCommandLineW(), &argc) };

            std::vector<std::string> char_arg_list(argc);
            for (int i = 0; i < argc; ++i)
                char_arg_list[i] = narrow(wchar_arg_list[i]);

            CommandLine()->CreateCmdLine(char_arg_list);
            LocalFree(wchar_arg_list);
        }
#else
        CommandLine()->CreateCmdLine(std::vector<std::string>(argv, argv + argc));
#endif
        CLauncher launcher;
        if (CommandLine()->FindParam("-debug"))
            launcher.SwitchDebugMode();
        return launcher.Run();
    }
    catch (const std::exception& e) {
        #ifdef _WIN32
        ::MessageBoxW(nullptr, widen(e.what()).c_str(), L"Error!",
            MB_OK | MB_ICONERROR);
        #else
        Error << e.what() << "!\n\n";
        #endif
        return 1;
    }
}

CLauncher::CLauncher() {
    std::cout << stc::true_color;
}

CLauncher::~CLauncher() {
    #ifdef _WIN32
    if (debugMode_)
        FreeConsole();
    #endif
}

int CLauncher::Run() {
    Msg << "Application successfully finished\n\n";
    return 0;
}

void CLauncher::SwitchDebugMode()
{
    if (!debugMode_)
    {
        #ifdef _WIN32
        FILE* fDummy;
        ::AllocConsole();
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONIN$", "r", stdin);
        std::cout.clear();
        std::clog.clear();
        std::cerr.clear();
        std::cin.clear();

        ::SetConsoleCP(CP_UTF8);
        ::SetConsoleOutputCP(CP_UTF8);

        // Making allow ansi escape
        DWORD dwMode = 0;
        HANDLE a{ ::GetStdHandle(STD_OUTPUT_HANDLE) };
        ::GetConsoleMode(a, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        ::SetConsoleMode(a, dwMode);
        std::cout << stc::true_color;
        #endif
    }
    #ifdef _WIN32
    else
        FreeConsole();
    #endif
    debugMode_ = !debugMode_;
}
