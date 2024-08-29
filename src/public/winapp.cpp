#ifdef PLATFORM_WINDOWS
    #include "unicode.hpp"
    #include "platform.hpp"

    #include <vector>
    #include <string_view>
    #include <filesystem>
    #include <fstream>
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

namespace resource_loader {

    PLATFORM_CLASS std::vector<char> ReadFile(const std::string_view filename) {
        static std::filesystem::path rootDir;

        if (rootDir.empty()) {
            wchar_t buffer[MAX_PATH] = { 0 };
            ::GetModuleFileNameW(NULL, buffer, MAX_PATH);
            rootDir = buffer;
            rootDir.remove_filename();
        }

        std::ifstream file((rootDir.wstring() + widen(filename)).c_str(), std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            file.close();
            return {};
        }

        std::size_t fileSize = static_cast<std::size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

} // resource_loader

#if 0
    #include "platform.hpp"
    #include "unicode.hpp"
    #include <windows.h>
    #include <string>

std::string getWinapiErrorMessage() {
    wchar_t* errorMsg = nullptr;
    ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        errorMsg, 0, nullptr
    );
    std::string finalMsg = narrow(errorMsg);
    ::LocalFree(errorMsg);
    return finalMsg;
}
#endif

#else
    #error
#endif
