#include <skylabs/public/filesystem.hpp>
#include <skylabs/public/sdl/filestream.hpp>
#include <skylabs/public/os.hpp>

namespace {
template <typename T>
std::vector<T> LoadAsGenericVector(std::string_view uri) {
    auto stream = Filesystem::LoadAsIO(uri);
    if (!stream) return {};

    const std::size_t totalBytes = stream->Size();
    if (totalBytes == 0) return {};

    const std::size_t count = totalBytes / sizeof(T);

    std::vector<T> result(count);
    stream->Read(result.data(), totalBytes);

    return result;
}
}

Filesystem& Filesystem::Instance() {
    static Filesystem instance;
    return instance;
}

Filesystem::Filesystem() {
#ifdef PLATFORM_ANDROID
    Mount("assets", "");
    Mount("assets", "assets:/");
    Mount("res", "");
#else
    Mount("assets", OS::PathJoin(OS::GetExecutableDirectory(), "assets"));
    Mount("assets", OS::GetExecutableDirectory());
    Mount("res", OS::GetExecutableDirectory());
#endif
}

void Filesystem::Mount(std::string_view scheme, std::string_view physicalPath) {
    m_mountPoints[std::string(scheme)].emplace_back(physicalPath);
}

std::string Filesystem::ResolvePath(std::string_view uri) const {
    std::size_t protocolEnd = uri.find("://");

    if (protocolEnd == std::string_view::npos) {
        return std::string(uri);
    }

    std::string scheme = std::string(uri.substr(0, protocolEnd));
    std::string_view relativePath = uri.substr(protocolEnd + 3);

    auto it = m_mountPoints.find(scheme);
    if (it != m_mountPoints.end()) {
        for (const auto& root : it->second) {
            std::string fullPath = root.empty() ? std::string(relativePath) : root + "/" + std::string(relativePath);

            SDL_PathInfo info;
            if (SDL_GetPathInfo(fullPath.c_str(), &info)) {
                return fullPath;
            }
        }
    }

    return "";
}

std::unique_ptr<IFileStream> Filesystem::LoadAsIO(std::string_view uri) {
    auto handle = SDL_IOFromFile(Filesystem::Instance().ResolvePath(uri).c_str(), "rb");
    if (!handle) return nullptr;
    return std::make_unique<SDL::CFileStream>(handle);
}

std::string Filesystem::LoadAsString(std::string_view uri) {
    auto stream = LoadAsIO(uri);
    if (!stream) return {};

    std::size_t size = stream->Size();
    if (size == 0) return {};

    std::string result;
    result.resize(size);
    stream->Read(result.data(), size);

    return result;
}

std::vector<std::byte> Filesystem::LoadAsVectorByte(std::string_view uri) {
    return LoadAsGenericVector<std::byte>(uri);
}

std::vector<std::uint8_t> Filesystem::LoadAsVector8(std::string_view uri) {
    return LoadAsGenericVector<std::uint8_t>(uri);
}

std::vector<std::uint32_t> Filesystem::LoadAsVector32(std::string_view uri) {
    return LoadAsGenericVector<std::uint32_t>(uri);
}
