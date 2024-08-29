#ifdef PLATFORM_POSIX
    #include <vector>
    #include <fstream>
    #include <filesystem>
    #include <string_view>

    #include "resourceloader.hpp"
    #include "platform.hpp"

namespace resource_loader {

std::vector<char> ReadFile(const std::string_view filename) {
    static std::filesystem::path rootDir;

    if (rootDir.empty()) {
        rootDir = std::filesystem::canonical("/proc/self/exe");
        rootDir.remove_filename();
    }

    std::ifstream file(rootDir.string() + filename.data(), std::ios::ate | std::ios::binary);

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
