#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <boost/nowide/convert.hpp>
#endif

#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/os.hpp"

namespace sk::os {
#ifdef PLATFORM_WINDOWS
std::string GetAppRoot() {
    static const std::string cachedPath = [] {
        std::wstring buffer(MAX_PATH, L'\0');
        while (true) {
            const DWORD size =
                GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

            if (size == 0) {
                log::Error("Failed to get executable directory: {}",
                           GetWindowsError(static_cast<std::uint32_t>(GetLastError())));

                return std::string { };
            }

            if (size < buffer.size()) {
                break;
            }

            buffer.resize(buffer.size() + MAX_PATH);
        }

        return boost::nowide::narrow(std::filesystem::path { buffer }.parent_path().wstring());
    }();

    return cachedPath;
}

std::string GetWindowsError(const std::uint32_t errorCode) {
    wchar_t* errorText = nullptr;
    const DWORD charCount =
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       nullptr, static_cast<DWORD>(errorCode), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       reinterpret_cast<LPWSTR>(&errorText), 0, nullptr);

    if (charCount == 0 || errorText == nullptr) {
        return fmt::format("0x{:08X}", errorCode);
    }

    const std::string narrowErrorText = boost::nowide::narrow(errorText);
    LocalFree(errorText);

    return narrowErrorText;
}
#else
std::string GetAppRoot() {
    static const std::string cachedPath = [] {
        std::error_code ec;
        const std::filesystem::path p = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (ec) {
            log::Error("Failed to get executable directory: {}", ec.message());
            return std::string { };
        }

        return p.parent_path().parent_path().string();
    }();

    return cachedPath;
}
#endif
}
