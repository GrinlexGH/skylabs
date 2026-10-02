#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/os.hpp"

#ifdef PLATFORM_WINDOWS
namespace {
std::string WideToUtf8(std::wstring_view wstr) {
    if (wstr.empty()) return { };

    const int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()),
                                               nullptr, 0, nullptr, nullptr);

    std::string str(sizeNeeded, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), sizeNeeded,
                        nullptr, nullptr);

    return str;
}
}
#endif

namespace sk::os {
[[nodiscard]] LocalTime GetLocalTime(const std::chrono::system_clock::time_point tp) noexcept {
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    LocalTime result { };

#ifdef PLATFORM_WINDOWS
    localtime_s(&result.tm, &t);
#else
    localtime_r(&t, &result.tm);
#endif

    char tempBuf[6] { };
    const std::size_t written = std::strftime(tempBuf, sizeof(tempBuf), "%z", &result.tm);

    if (written > 0) {
        std::memcpy(result.tzOffset.data(), tempBuf, written);
    }

    return result;
}

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

        return WideToUtf8(std::filesystem::path { buffer }.parent_path().wstring());
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

    const std::string narrowErrorText = WideToUtf8(errorText);
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
