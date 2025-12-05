#include <skylabs/public/resource_system.hpp>

#include <skylabs/public/os.hpp>
#include <skylabs/public/logging.hpp>

#include <boost/nowide/fstream.hpp>

namespace {
std::string GetRelativeResourcePath(const ResourceSystem::ResourceType type, const std::string_view relativePath) {
    std::string path = OS::GetProgramPath();
    path.reserve(relativePath.size() + 10);
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
[[nodiscard]] PUBLIC_CLASS std::vector<char> LoadBinary(const ResourceType type, const std::string_view relativePath) {
    const std::string path = GetRelativeResourcePath(type, relativePath);

    boost::nowide::ifstream file(path.c_str(), std::ios_base::ate | std::ios_base::binary);

    if (!file.is_open()) {
        if (file.fail()) {
            throw std::runtime_error("Failed to open file: " + path);
        }
        Log::Warning("Couldn't open file: {}", path);
        return {};
    }

    const std::streampos fileSize = file.tellg();

    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
}
