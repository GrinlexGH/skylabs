#include <skylabs/public/os.hpp>

namespace OS {
#ifdef PLATFORM_WINDOWS
PUBLIC_CLASS std::string GetExecutableDirectory() {
    static const std::string cachedPath = [] {
        std::wstring buffer(MAX_PATH, L'\0');
        while (true) {
            if (const DWORD size = GetModuleFileNameW(nullptr, buffer.data(), buffer.size());
                size < buffer.size()
            ) { break; }

            buffer.resize(buffer.size() + MAX_PATH);
        }
        return boost::nowide::narrow(std::filesystem::path { buffer }.parent_path().wstring());
    }();

    return cachedPath;
}

PUBLIC_CLASS std::string GetWindowsError(const DWORD errorCode) {
    wchar_t* errorText = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_MAX_WIDTH_MASK,
        nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&errorText), 0, nullptr
    );

    const std::string narrowErrorText = boost::nowide::narrow(errorText);
    LocalFree(errorText);

    return narrowErrorText;
}
#else
PUBLIC_CLASS std::string GetExecutableDirectory() {
    static const std::string cachedPath = [] {
        std::error_code ec;
        std::filesystem::path p = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (ec) {
            return std::string();
        }

        return p.parent_path().parent_path().string();
    }();

    return cachedPath;
}
#endif
}
