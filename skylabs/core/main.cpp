#include <skylabs/core/launcher.hpp>
#include <skylabs/public/dll_export.hpp>

#include <span>

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <boost/nowide/convert.hpp>

#include <skylabs/public/logging.hpp>

namespace {
std::string GetLastErrorMessage() {
    wchar_t* errorMsg = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&errorMsg),
        0, nullptr
    );

    std::string finalMsg = boost::nowide::narrow(errorMsg);
    LocalFree(errorMsg);
    return finalMsg;
}

void EnableVTP() {
    const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) {
        Log::Error("Failed to get stdout handle: {}", GetLastErrorMessage());
        return;
    }

    constexpr DWORD VTP_FLAGS = ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;

    DWORD originalMode = 0;
    if (!GetConsoleMode(handle, &originalMode)) {
        Log::Error("Failed to get console mode: {}", GetLastErrorMessage());
        return;
    }

    if ((originalMode & VTP_FLAGS) == VTP_FLAGS) {
        Log::Debug("Virtual terminal processing already set for STD_OUTPUT.");
        return;
    }

    if (!SetConsoleMode(handle, originalMode | VTP_FLAGS)) {
        Log::Error("Failed to set virtual terminal processing flags: {}", GetLastErrorMessage());
    }
}
}
#endif

extern "C" DLL_EXPORT int CoreMain(int /*argc*/, char* /*argv*/[]) {
#ifdef PLATFORM_WINDOWS
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    EnableVTP();
#endif

    CLauncher launcher;
    launcher.Run();

    return 0;
}
