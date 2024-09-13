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

#else
    #error
#endif
