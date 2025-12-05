#include <skylabs/public/os.hpp>

#include <filesystem>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <boost/nowide/convert.hpp>
#endif

namespace OS {
#ifdef PLATFORM_WINDOWS

std::string GetProgramPath() {
    static const std::string programPath = [] {
        std::wstring out(100, L'\0');
        DWORD size;
        while (true) {
            size = GetModuleFileNameW(nullptr, out.data(), static_cast<DWORD>(out.size()));
            if (size < out.size())
                break;
            out.resize(out.size() + 100);
        }
        out.resize(size);
        return boost::nowide::narrow(std::filesystem::path(std::move(out)).parent_path().wstring());
    }();

    return programPath;
}

#elifdef PLATFORM_UNIX

std::string GetProgramPath() {
    static std::filesystem::path programPath = std::filesystem::canonical("/proc/self/exe").remove_filename();
    return programPath.string();
}

#endif
}
