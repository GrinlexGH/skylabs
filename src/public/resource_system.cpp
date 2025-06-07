#include "resource_system.hpp"

#include "os.hpp"

#include <nowide/fstream.hpp>
#include <cstring>
#include <format>

#include "logging.hpp"
struct MyString : std::string {
    using std::string::string;
        MyString(const std::string& other) : std::string(other) {
        Log::Info("Copied from std::string");
    }

    MyString(std::string&& other) noexcept : std::string(std::move(other)) {
        Log::Info("Moved from std::string");
    }
    MyString(MyString&&) { Log::Info("Moved"); }
    MyString(const MyString&) { Log::Info("Copied"); }
};

namespace {
MyString GetRelativeResourcePath(const ResourceSystem::ResourceType type, const char* relativePath) {
    MyString path = OS::GetProgramPath();
    path.reserve(std::strlen(relativePath) + 10);
    switch (type) {
        case ResourceSystem::ResourceType::eShader: {
            path += "/shaders/";
        } break;
    }
    path += relativePath;
    return path;
}
}

namespace ResourceSystem {
[[nodiscard]] PUBLIC_CLASS std::vector<char> LoadBinary(const ResourceType type, const char* relativePath) {
    MyString path = GetRelativeResourcePath(type, relativePath);

    nowide::ifstream file(path.c_str(), std::ios_base::ate | std::ios_base::binary);
    file.exceptions(std::ios_base::failbit | std::ios_base::badbit);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    const std::streampos fileSize = file.tellg();

    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
}
