#ifdef _WIN32

#include <Windows.h>
#include <filesystem>
#include "charconverters.hpp"
#include "baseapplication.hpp"
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
        BaseApplication::Init();
        BaseApplication::AddToEnvPATH(BaseApplication::rootDir.string() + "/bin");
        std::string a;
        size_t envPathLen;
        std::string envPath;
        getenv_s(&envPathLen, NULL, 0, "PATH");
        if (envPathLen > 0) {
            envPath.reserve(envPathLen);
        }
        else {
            throw Exception("Failed to find PATH");
        }
        getenv_s(
            &envPathLen,
            envPath.data(),
            envPathLen,
            "PATH"
        );
        //MessageBox(NULL, CharConverters::UTF8ToWideStr(envPath.c_str()).c_str(), L"", MB_OK);
        return 0;
    }
    catch (const Exception& e) {
        MessageBox(NULL, CharConverters::UTF8ToWideStr(e.what()).c_str(), L"", MB_OK);
        //OutputDebugString(CharConverters::UTF8ToWideStr(e.what()).c_str());
        return 1;
    }
}

#endif

