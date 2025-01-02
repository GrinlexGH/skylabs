#ifdef PLATFORM_WINDOWS
#include <windows.h>
#elif defined(PLATFORM_LINUX)
#include <dlfcn.h>
#include <iostream>
#else
#error
#endif

#include <cassert>
#include <filesystem>
#include <stdexcept>
#include <string>

using main_t = int (*)(int argc, char* argv[]);

#ifdef PLATFORM_WINDOWS

namespace
{
std::string Narrow(const std::wstring_view wideStr) {
    if (wideStr.empty()) {
        return {};
    }

    const int len = WideCharToMultiByte(
        CP_UTF8,
        0,
        wideStr.data(),
        static_cast<int>(wideStr.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    std::string out(len, 0);
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wideStr.data(),
        static_cast<int>(wideStr.size()),
        out.data(),
        len,
        nullptr,
        nullptr
    );

    return out;
}

std::wstring Widen(const std::string_view narrowStr) {
    if (narrowStr.empty()) {
        return {};
    }

    const int len = MultiByteToWideChar(
        CP_UTF8,
        0,
        narrowStr.data(),
        static_cast<int>(narrowStr.size()),
        nullptr,
        0
    );

    std::wstring out(len, 0);
    MultiByteToWideChar(
        CP_UTF8,
        0,
        narrowStr.data(),
        static_cast<int>(narrowStr.size()),
        out.data(),
        len
    );

    return out;
}

std::string GetWinApiErrorMessage() {
    wchar_t* errorMsg = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&errorMsg),
        0,
        nullptr
    );

    std::string finalMsg = Narrow(errorMsg);

    LocalFree(errorMsg);

    return finalMsg;
}
}

int WINAPI wWinMain(
    [[maybe_unused]] _In_ HINSTANCE hInstance,
    [[maybe_unused]] _In_opt_ HINSTANCE hPrevInstance,
    [[maybe_unused]] _In_ LPWSTR lpCmdLine,
    [[maybe_unused]] _In_ int nShowCmd
) {
    try {
        std::filesystem::path rootDir;
        {
            wchar_t buffer[MAX_PATH] = {};
            GetModuleFileNameW(nullptr, buffer, MAX_PATH);
            rootDir = buffer;
        }
        rootDir.remove_filename();

        const auto core = LoadLibraryExW(
            (rootDir.wstring() + L"\\bin\\core.dll").c_str(),
            nullptr,
            LOAD_WITH_ALTERED_SEARCH_PATH
        );
        if (!core) {
            throw std::runtime_error(
                "Failed to load core library: " +
                GetWinApiErrorMessage()
            );
        }

        const auto main = reinterpret_cast<main_t>(GetProcAddress(core, "CoreInit"));
        if (!main) {
            throw std::runtime_error(
                "Failed to load the core entry proc: " +
                GetWinApiErrorMessage()
            );
        }

        // converts wide argv to narrow argv
        int argc = 0;
        wchar_t** wArgv = CommandLineToArgvW(GetCommandLineW(), &argc);

        const auto argv = new char*[argc];
        for (int i = 0; i < argc; ++i) {
            std::string narrowStr = Narrow(wArgv[i]);
            const std::size_t argSize = narrowStr.size() + 1;

            argv[i] = new char[argSize];
            strcpy_s(argv[i], argSize, narrowStr.c_str());
        }
        LocalFree(wArgv);

        // call real main with normal arguments, not schizophrenia from windows
        const int ret = main(argc, argv);

        for (int i = 0; i < argc; ++i) {
            delete[] argv[i];
        }
        delete[] argv;

        FreeLibrary(core);
        return ret;
    } catch (const std::exception& e) {
        MessageBeep(MB_ICONERROR);
        MessageBoxW(
            nullptr,
            Widen(e.what()).c_str(),
            L"Error!",
            MB_OK | MB_ICONERROR
        );
        return 1;
    }
}

#elif defined(PLATFORM_LINUX)

int main(int argc, char** argv) {
    try {
        std::filesystem::path rootDir = std::filesystem::canonical("/proc/self/exe");
        rootDir.remove_filename();

        void* lib = dlopen((rootDir.string() + "/bin/libcore.so").c_str(), RTLD_NOW);
        if (!lib) {
            throw std::runtime_error(std::string("failed open library: ") + dlerror() + "!\n");
        }

        auto main = (CoreMain_t)dlsym(lib, "CoreInit");
        if (!main) {
            throw std::runtime_error(
                std::string("Failed to load the launcher entry proc: ") +
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
