#include "os.hpp"

#include <filesystem>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <conio.h>
#include <nowide/convert.hpp>
#endif

namespace OS {
#ifdef PLATFORM_WINDOWS
std::string GetProgramPath() {
    static std::string programPath = [] {
        std::wstring out(MAX_PATH, '\0');
        DWORD size = GetModuleFileNameW(nullptr, out.data(), MAX_PATH);
        if (size == 0) {
            throw std::runtime_error("Failed to get executable path!");
        }

        while (size == out.size()) {
            out.resize(out.size() * 1.5);
            size = GetModuleFileNameW(nullptr, out.data(), static_cast<DWORD>(out.size()));
        }
        out.shrink_to_fit();
        return nowide::narrow(std::filesystem::path(std::move(out)).parent_path().wstring());
    }();

    return programPath;
}
#elif defined(PLATFORM_UNIX)
std::string GetProgramPath() {
    static std::filesystem::path programPath = std::filesystem::canonical("/proc/self/exe").remove_filename();
    return programPath.string();
}
#endif
}
