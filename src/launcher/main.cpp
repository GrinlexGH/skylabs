#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <dlfcn.h>
#include <iostream>
#else
#error
#endif

#include <filesystem>
#include <string>
#include <stdexcept>

#ifdef WIN32
using CoreMain_t = int (*)(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nShowCmd);
#elif defined(__linux__)
using CoreMain_t = int (*)(int argc, char** argv);
#endif

// Microsoft, fuck you
#ifdef _WIN32

static std::string narrow(std::wstring_view wstr) {
    if (wstr.empty())
        return {};
    int len = ::WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0,
        NULL, NULL);
    std::string out(len, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &out[0], len,
        NULL, NULL);
    return out;
}

static std::wstring widen(std::string_view str) {
    if (str.empty())
        return {};
    int len = ::MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring out(len, 0);
    ::MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &out[0], len);
    return out;
}

static std::string getWinapiErrorMessage() {
    wchar_t* errorMsg;
    ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsg,
        0, NULL);
    std::string finalMsg{ narrow(errorMsg) };
    ::LocalFree(errorMsg);
    return finalMsg;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    try
    {
        std::filesystem::path rootDir{ };
        {
            wchar_t buffer[MAX_PATH]{ 0 };
            if (!::GetModuleFileNameW(NULL, buffer, MAX_PATH))
                throw std::runtime_error("GetModuleFileNameW call failed: " + getWinapiErrorMessage() + "!\n");
            rootDir = narrow(buffer);
        }
        rootDir.remove_filename();

        std::size_t curPathLen{ 0 };
        std::wstring newPath;
        _wgetenv_s(&curPathLen, NULL, 0, L"PATH");
        if (curPathLen > 0) {
            auto currentPath = std::make_unique<wchar_t[]>(curPathLen);
            _wgetenv_s(&curPathLen, currentPath.get(), curPathLen, L"PATH");
            newPath = widen(rootDir.string()) + L';' + currentPath.get();
        }
        else
            newPath = widen(rootDir.string()) + L";";

        _wputenv_s(L"PATH", newPath.c_str());

        void* core{ ::LoadLibraryExW((widen(rootDir.string()) + L"\\bin\\core.dll").c_str(), NULL,
            LOAD_WITH_ALTERED_SEARCH_PATH) };
        if (!core)
            throw std::runtime_error("Failed to load core library: " + getWinapiErrorMessage() + "!\n");

        auto main = (CoreMain_t)(void*)GetProcAddress((HINSTANCE)core, "CoreInit");
        if (!main)
            throw std::runtime_error("Failed to load the core entry proc: " + getWinapiErrorMessage() + "!\n");
        int ret = main(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
        ::FreeLibrary((HMODULE)core);
        return ret;
    }
    catch (const std::exception& e)
    {
        ::MessageBoxW(nullptr, widen(e.what()).c_str(), L"Error!",
            MB_OK | MB_ICONERROR);
        return 1;
    }
}

// Torvalds, use c++ in linux kernel pls
#elif defined(__linux__)

int main(int argc, char** argv)
{
    try
    {
        std::filesystem::path rootDir = std::filesystem::canonical("/proc/self/exe");
        rootDir.remove_filename();
        void* lib = dlopen((rootDir.string() + "/bin/libcore.so").c_str(), RTLD_NOW);
        if (!lib)
            throw std::runtime_error(std::string{ "failed open library: " } +
            dlerror() + "!\n");
        auto main = (CoreMain_t)dlsym(lib, "CoreInit");
        if (!main)
            throw std::runtime_error(std::string{ "Failed to load the launcher entry proc: " } +
                dlerror() + "!\n");
        int ret = main(argc, argv);
        dlclose(lib);
        return ret;
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }
}

#endif
