#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(PLATFORM_UNIX)
#include <dlfcn.h>
#include <iostream>
#else
#error
#endif

#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>

using main_t = int (*)(int argc, char* argv[]);

#ifdef PLATFORM_WINDOWS

#include <nowide/args.hpp>
#include <nowide/convert.hpp>

namespace {
std::string GetLastErrorMessage() {
    wchar_t* errorMsg = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&errorMsg), 0,
        nullptr
    );

    std::string finalMsg { nowide::narrow(errorMsg) };

    LocalFree(errorMsg);

    return finalMsg;
}

std::wstring GetProgramPath() {
    std::wstring out(100, L'\0');
    DWORD size;

    while (true) {
        size = GetModuleFileNameW(nullptr, out.data(), static_cast<DWORD>(out.size()));
        if (size < out.size())
            break;

        out.resize(out.size() + 100);
    }

    out.resize(size);
    return out;
}

class CLibrary
{
public:
    CLibrary() = delete;
    explicit CLibrary(const wchar_t* path) : m_handle(LoadLibraryExW(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH)) {
        if (!m_handle) {
            throw std::runtime_error {
                std::format("Failed to load library:\n{}\n\n{}\n", nowide::narrow(path), GetLastErrorMessage())
            };
        }
    }
    CLibrary(const CLibrary&) = delete;
    CLibrary(CLibrary&&) = delete;
    CLibrary& operator=(const CLibrary&) = delete;
    CLibrary& operator=(CLibrary&&) = delete;
    ~CLibrary() { if (m_handle) { FreeLibrary(m_handle); } }

    void* GetFunctionAddress(const char* name) const {
        const auto func = reinterpret_cast<void*>(GetProcAddress(m_handle, name));
        if (!func) {
            throw std::runtime_error {
                std::format("Failed to load library function:\n{}\n\n{}\n", name, GetLastErrorMessage())
            };
        }
        return func;
    }

private:
    HMODULE m_handle;
};
}

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nShowCmd*/) {
    try {
        std::filesystem::path rootDir { GetProgramPath() };
        rootDir.remove_filename();

        const std::filesystem::path libCorePath { rootDir / L"bin" / L"core.dll" };

        const CLibrary core(libCorePath.c_str());
        const auto main = reinterpret_cast<main_t>(core.GetFunctionAddress("CoreMain"));

        // converts wide argv to narrow argv
        int argc = 0;
        char** argv = nullptr;
        const nowide::args fix(argc, argv);

        try {
            // call real main with normal arguments, not schizophrenia from windows
            const int ret = main(argc, argv);
            return ret;
        } catch (const std::exception& e) {
            MessageBoxW(
                nullptr,
                nowide::widen(e.what()).c_str(),
                L"Error!",
                MB_OK | MB_ICONERROR
            );
            return 1;
        }
    } catch (const std::exception& e) {
        MessageBoxW(
            nullptr,
            nowide::widen(e.what()).c_str(),
            L"Error!",
            MB_OK | MB_ICONERROR
        );
        return 1;
    }
}

int main() {
    // Dummy main for console in debug
    return wWinMain(GetModuleHandleW(nullptr), nullptr, GetCommandLineW(), SW_SHOWNORMAL);
}

#elifdef PLATFORM_UNIX

class CLibrary
{
public:
    CLibrary() = delete;
    explicit CLibrary(const char* path) : m_handle(dlopen(path, RTLD_LAZY)) {
        if (!m_handle) {
            throw std::runtime_error {
                std::format("Failed to load library:\n{}\n", dlerror())
            };
        }
    }
    CLibrary(const CLibrary&) = delete;
    CLibrary(CLibrary&&) = delete;
    CLibrary& operator=(const CLibrary&) = delete;
    CLibrary& operator=(CLibrary&&) = delete;
    ~CLibrary() { if(m_handle) { dlclose(m_handle); } }

    void* GetFunctionAddress(const char* name) const {
        void* const func = dlsym(m_handle, name);
        if (const char* error = dlerror(); error != nullptr) {
            throw std::runtime_error {
                std::format("Failed to load library function:\n{}\n", error)
            };
        }
        return func;
    }

private:
    void* m_handle;
};

#ifdef PLATFORM_ANDROID

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

__attribute__((visibility("default"))) int main(int argc, char* argv[]) {
    SDL_Log("Skylabs is starting...");

    try {
        const std::string libCorePath = "libcore.so";

        const CLibrary core(libCorePath.c_str());
        const auto coreMain = reinterpret_cast<main_t>(core.GetFunctionAddress("CoreMain"));

        try {
            const int ret = coreMain(argc, argv);
            return ret;
        } catch (const std::exception& e) {
            SDL_LogError(0, "%s", e.what());
        }

        return 1;
    } catch (const std::exception& e) {
        SDL_LogError(0, "%s", e.what());
        return 1;
    }
}

#else

int main(int argc, char* argv[]) {
    try {
        std::filesystem::path rootDir = std::filesystem::canonical("/proc/self/exe");
        rootDir.remove_filename();

        const std::string libCorePath = rootDir / "bin" / "libcore.so";

        const CLibrary core(libCorePath.c_str());
        const auto coreMain = reinterpret_cast<main_t>(core.GetFunctionAddress("CoreMain"));

        try {
            const int ret = coreMain(argc, argv);
            return ret;
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n' << std::flush;
        }

        return 1;
    } catch (const std::exception& e) {
        std::cout << e.what() << '\n' << std::flush;
        return 1;
    }
}

#endif

#endif
