#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <console.hpp>
#include <dlfcn.h>
#include <iostream>
#include <string>
#else
#error
#endif

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "application.hpp"
#include "commandline.hpp"
#include "unicode.hpp"
#include "utilities.hpp"
#include "console.hpp"

#ifdef WIN32
using CoreMain_t = int (*)(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                           LPWSTR lpCmdLine, int nShowCmd);
#elif defined(__linux__)
using CoreMain_t = int (*)(int argc, char **argv);
#endif

#ifdef _WIN32

class CLauncher final : public IApplication
{
public:
    void Init() override;
    void SwitchDebugMode() override;
};

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    try
    {
        {
            wchar_t **wchar_arg_list;
            int argc;
            wchar_arg_list = CommandLineToArgvW(GetCommandLine(), &argc);
            if (wchar_arg_list == NULL)
                throw std::runtime_error("Unable to get argv!");

            std::vector<std::string> char_arg_list(argc);
            for (int i = 0; i < argc; ++i)
                char_arg_list[i] = narrow(wchar_arg_list[i]).data();

            CommandLine()->CreateCmdLine(argc, char_arg_list);
            LocalFree(wchar_arg_list);
        }

        CLauncher launcher;
        if (CommandLine()->CheckParm("-debug"))
            launcher.SwitchDebugMode();

        launcher.Init();
        AddLibSearchPath(launcher.GetRootDir().string() + "bin");
        std::string corePath =
            launcher.GetRootDir().string() + "\\bin\\core.dll";
        void *core = LoadLib(corePath);
        auto main = (CoreMain_t)(void *)GetProcAddress((HINSTANCE)core, "CoreInit");
        int ret = main(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
        FreeLibrary((HMODULE)core);
        std::cin.get();
        return ret;
    }
    catch (const std::exception &e)
    {
        MessageBox(nullptr, widen(e.what()).c_str(), L"Error!",
                   MB_OK | MB_ICONERROR);
        return 1;
    }
}

void CLauncher::Init()
{
    Msg("Initializing application...\n");
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    Msg("Console code page: %d\n", CP_UTF8);

    wchar_t buffer[MAX_PATH];

    if (GetModuleFileName(nullptr, buffer, MAX_PATH) == MAX_PATH) {
        wchar_t *errorMsg;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                      nullptr, GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsg,
                      0, nullptr);
        throw std::runtime_error(narrow(errorMsg).c_str());
    }

    rootDir_ = narrow(buffer);
    rootDir_.remove_filename();
    Msg("rootDir == %s\n", rootDir_.string().c_str());
    Msg("Initializing finished.\n\n");
}

void CLauncher::SwitchDebugMode()
{
    if (!debugMode_) {
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
        freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    } else
        FreeConsole();
    debugMode_ = !debugMode_;
}

#elif defined(__linux__)
int main(int argc, char **argv)
{
    try
    {
        Application::Init();
        Application::AddLibSearchPath("/bin");
        std::string corePath =
            Application::rootDir.parent_path().string() + "/bin/libcore.so";
        void *core = Application::LoadLib(corePath);
        auto main = (CoreMain_t)dlsym(core, "CoreInit");
        if (!main)
        {
            console::Msg("Failed to load the launcher entry proc\n");
            return 0;
        }
        int ret = main(argc, argv);
        dlclose(core);
        return ret;
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }
}
#endif
