#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(PLATFORM_UNIX)
#include <dlfcn.h>
#include <iostream>
#else
#error
#endif

#include <string>
#include <stdexcept>
#include <format>
#include <filesystem>

#include <nowide/args.hpp>
#include <nowide/convert.hpp>

using main_t = int (*)(int argc, char* argv[]);

#ifdef PLATFORM_WINDOWS

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

void* GetFunctionAddress(HINSTANCE lib, LPCSTR funcName) {
    return reinterpret_cast<void*>(GetProcAddress(lib, funcName));
}
}

int WINAPI WinMain(
    _In_ HINSTANCE /*hInstance*/,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPSTR /*lpCmdLine*/,
    _In_ int /*nShowCmd*/
) {
    try {
        std::filesystem::path rootDir { GetProgramPath() };
        rootDir.remove_filename();

        const std::filesystem::path libCorePath { rootDir / L"bin" / L"core.dll" };

        const HMODULE core = LoadLibraryExW(libCorePath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!core) {
            throw std::runtime_error {
                std::format("Failed to load library:\n{}\n\n{}\n", nowide::narrow(libCorePath.c_str()), GetLastErrorMessage())
            };
        }

        const auto main = reinterpret_cast<main_t>(GetFunctionAddress(core, "CoreMain"));
        if (!main) {
            throw std::runtime_error {
                std::format("Failed to load library entry point:\n{}\n\n{}\n", nowide::narrow(libCorePath.c_str()), GetLastErrorMessage())
            };
        }

        // converts wide argv to narrow argv
        int argc = 0;
        char** argv = nullptr;
        nowide::args fix(argc, argv);

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

#elif defined(PLATFORM_UNIX)

int main(int argc, char** argv) {
    try {
        std::filesystem::path rootDir = std::filesystem::canonical("/proc/self/exe");
        rootDir.remove_filename();

        const std::string libCorePath = rootDir / "bin" / "libcore.so";

        void* lib = dlopen(libCorePath.c_str(), RTLD_NOW);
        if (!lib) {
            throw std::runtime_error(std::format("Failed load library:\n{}\n\n{}\n", libCorePath, dlerror()));
        }

        auto main = reinterpret_cast<main_t>(dlsym(lib, "CoreMain"));
        if (!main) {
            throw std::runtime_error(
                std::format("Failed to load library entry point:\n{}\n", dlerror())
            );
        }

        try {
            int ret = main(argc, argv);
            dlclose(lib);
            return ret;
        } catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        }

        dlclose(lib);
        return 1;
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}

#endif
