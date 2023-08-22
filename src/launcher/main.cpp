#ifdef _WIN32

#include <Windows.h>
#include <filesystem>
#include "string.hpp"
#include "exception.hpp"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(nShowCmd);

    try {
        std::filesystem::path rootDir = std::filesystem::current_path();

        // Get the current PATH environment variable
        std::wstring currentPathEnv;
        DWORD currentPathSize = GetEnvironmentVariable(L"PATH", nullptr, 0);
        if (currentPathSize > 0) {
            currentPathEnv.resize(currentPathSize);
            GetEnvironmentVariable(L"PATH", &currentPathEnv[0], currentPathSize);
        }
        else {
            throw std::runtime_error("Failed to find PATH");
        }

        // Construct the new PATH
        std::wstring newPathEnv = rootDir.wstring() + L"/bin" + (currentPathEnv.empty() ? L"" : L";" + currentPathEnv);

        // Update the PATH environment variable
        if (!SetEnvironmentVariable(L"PATH", newPathEnv.c_str())) {
            throw std::runtime_error("Failed to set PATH environment variable");
        }

        String s(L"Hello");
        

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

#endif

