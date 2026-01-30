#include <skylabs/public/resource_system.hpp>

#include <skylabs/public/os.hpp>
#include <skylabs/public/logging.hpp>

#include <boost/nowide/fstream.hpp>

namespace {
std::filesystem::path GetResourcePath(const ResourceSystem::ResourceType type, const std::string_view relativePath) {
    using enum ResourceSystem::ResourceType;

    const char* folder = "";
    switch (type) {
        case eShader: folder = "shaders"; break;
        default: break;
    }

    return OS::GetExecutableDirectory() / folder / relativePath;
}
}

namespace ResourceSystem {
[[nodiscard]] PUBLIC_CLASS std::vector<char> LoadBinary(const ResourceType type, const std::string_view relativePath) {
    const std::filesystem::path path = GetResourcePath(type, relativePath);

    boost::nowide::ifstream file(path, std::ios_base::ate | std::ios_base::binary);

    if (!file.is_open()) {
        auto s = path.u8string();
        std::string fileName(s.begin(), s.end());
        Log::Warning("Couldn't open file: {}", fileName);
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
