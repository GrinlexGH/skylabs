#ifdef PLATFORM_WINDOWS
    #include <Windows.h>
#elif defined(PLATFORM_POSIX)
    #include <dlfcn.h>
    #include <iostream>
#else
    #error
#endif

#include <cstddef>
#include <cassert>
#include <filesystem>
#include <stdexcept>
#include <string>

#ifdef PLATFORM_WINDOWS
using CoreMain_t = int (*)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd);
#elif defined(PLATFORM_POSIX)
using CoreMain_t = int (*)(int argc, char** argv);
#endif

#ifdef PLATFORM_WINDOWS

static std::string narrow(const std::wstring_view wstr) {
    if (wstr.empty()) {
        return {};
    }
    int len = ::WideCharToMultiByte(
        CP_UTF8, 0, &wstr[0], (int)wstr.size(),
        nullptr, 0, nullptr, nullptr
    );
    std::string out(len, 0);
    ::WideCharToMultiByte(
        CP_UTF8, 0, &wstr[0], (int)wstr.size(),
        &out[0], len, nullptr, nullptr
    );
    return out;
}

static std::wstring widen(const std::string_view str) {
    if (str.empty()) {
        return {};
    }
    int len = ::MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring out(len, 0);
    ::MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &out[0], len);
    return out;
}

static std::string getWinapiErrorMessage() {
    wchar_t* errorMsg = nullptr;
    ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&errorMsg, 0, nullptr
    );
    std::string finalMsg = narrow(errorMsg);
    ::LocalFree(errorMsg);
    return finalMsg;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
    try {
        std::filesystem::path rootDir;
        {
            wchar_t buffer[MAX_PATH] = { 0 };
            ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
            rootDir = narrow(buffer);
        }
        rootDir.remove_filename();

        auto core = ::LoadLibraryExW(
            (widen(rootDir.string()) + L"\\bin\\core.dll").c_str(),
            nullptr,
            LOAD_WITH_ALTERED_SEARCH_PATH
        );
        if (!core) {
            throw std::runtime_error(
                "Failed to load core library: " +
                getWinapiErrorMessage()
            );
        }

        auto main = (CoreMain_t)(void*)GetProcAddress(core, "CoreInit");
        if (!main) {
            throw std::runtime_error(
                "Failed to load the core entry proc: " +
                getWinapiErrorMessage()
            );
        }

        int ret = main(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
        ::FreeLibrary(core);
        return ret;
    } catch (const std::exception& e) {
        ::MessageBeep(MB_ICONERROR);
        ::MessageBoxW(nullptr, widen(e.what()).c_str(), L"Error!", MB_OK | MB_ICONERROR);
        return 1;
    }
}

#elif defined(PLATFORM_POSIX)

int main(int argc, char** argv) {
    try {
        std::filesystem::path rootDir =
            std::filesystem::canonical("/proc/self/exe");
        rootDir.remove_filename();
        void* lib =
            dlopen((rootDir.string() + "/bin/libcore.so").c_str(), RTLD_NOW);
        if (!lib) {
            throw std::runtime_error(std::string { "failed open library: " } + dlerror() + "!\n");
        }
        auto main = (CoreMain_t)dlsym(lib, "CoreInit");
        if (!main) {
            throw std::runtime_error(
                std::string { "Failed to load the launcher entry proc: " } +
                dlerror()
            );
        }
        int ret = main(argc, argv);
        dlclose(lib);
        return ret;
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}

#endif
