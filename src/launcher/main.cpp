#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(PLATFORM_UNIX)
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

namespace {
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

class CArgFix
{
public:
    CArgFix(int& argc, char**& argv) :
    m_oldArgc(argc), m_oldArgv(argv), m_oldArgcPtr(&argc), m_oldArgvPtr(&argv) {
        FixArgs(argc, argv);
    }

    CArgFix(const CArgFix&) = delete;
    CArgFix(CArgFix&& other) = delete;
    CArgFix& operator=(const CArgFix&) = delete;
    CArgFix& operator=(CArgFix&& other) = delete;

    ~CArgFix() {
        if (m_oldArgcPtr)
            *m_oldArgcPtr = m_oldArgc;
        if (m_oldArgvPtr)
            *m_oldArgvPtr = m_oldArgv;
    }

private:
    class CWArgv
    {
    public:
        CWArgv() { m_ptr = CommandLineToArgvW(GetCommandLineW(), &m_argc); }

        ~CWArgv() {
            if (m_ptr) {
                LocalFree(m_ptr);
            }
        }

        CWArgv(const CWArgv&) = delete;
        CWArgv(CWArgv&& other) = delete;
        CWArgv& operator=(const CWArgv&) = delete;
        CWArgv& operator=(CWArgv&& other) = delete;

        [[nodiscard]] int size() const { return m_argc; }
        explicit operator bool() const { return m_ptr != nullptr; }
        const wchar_t* operator[](const std::size_t i) const { return m_ptr[i]; }

    private:
        wchar_t** m_ptr;
        int m_argc;
    };

    void FixArgs(int& argc, char**& argv) {
        const CWArgv wArgv;
        if (!wArgv) {
            throw std::runtime_error("Could not get command line!");
        }

        m_args.resize(wArgv.size() + 1, nullptr);
        m_argValues.resize(wArgv.size());

        for (int i = 0; i < wArgv.size(); i++) {
            m_argValues[i] = Narrow(wArgv[i]);
            m_args[i] = m_argValues[i].data();
        }

        argc = wArgv.size();
        argv = m_args.data();
    }

    std::vector<char*> m_args;
    std::vector<std::string> m_argValues;

    int m_oldArgc;
    char** m_oldArgv;

    int* m_oldArgcPtr;
    char*** m_oldArgvPtr;
};

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
        // Get exe directory for loading \\bin\\core.dll
        std::filesystem::path rootDir;
        {
            wchar_t buffer[MAX_PATH] {};
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
        char** argv = nullptr;
        CArgFix fix(argc, argv);

        // call real main with normal arguments, not schizophrenia from windows
        const int ret = main(argc, argv);

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

#elif defined(PLATFORM_UNIX)

int main(int argc, char** argv) {
    try {
        std::filesystem::path rootDir = std::filesystem::canonical("/proc/self/exe");
        rootDir.remove_filename();

        void* lib = dlopen((rootDir.string() + "/bin/libcore.so").c_str(), RTLD_NOW);
        if (!lib) {
            throw std::runtime_error(std::string("failed open library: ") + dlerror() + "!\n");
        }

        auto main = (main_t)dlsym(lib, "CoreInit");
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
