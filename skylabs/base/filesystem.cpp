#include "skylabs/base/filesystem.hpp"

namespace {
template <typename T>
std::vector<T> LoadAsGenericVector(const sk::filesystem::Filesystem& fs, std::string_view uri) {
    const auto stream = fs.LoadAsIO(uri);
    if (!stream) return { };

    const std::size_t totalBytes = stream->Size();
    if (totalBytes == 0) return { };

    std::vector<T> result(totalBytes / sizeof(T));
    stream->Read(result.data(), totalBytes);
    return result;
}
}

namespace sk::filesystem {
Filesystem::Filesystem(std::unique_ptr<IFilesystemBackend> backend) : m_backend(std::move(backend)) { }

void Filesystem::Mount(const std::string_view scheme, std::string_view physicalPath) {
    m_mountPoints[std::string(scheme)].emplace_back(physicalPath);
}

std::string Filesystem::ResolvePath(std::string_view uri) const {
    const std::size_t protocolEnd = uri.find("://");
    if (protocolEnd == std::string_view::npos) {
        return std::string(uri);
    }

    const std::string scheme(uri.substr(0, protocolEnd));
    const std::string_view relativePath = uri.substr(protocolEnd + 3);

    if (const auto it = m_mountPoints.find(scheme); it != m_mountPoints.end()) {
        for (const std::string& root : it->second) {
            std::string fullPath =
                root.empty() ? std::string(relativePath) : root + "/" + std::string(relativePath);
            if (m_backend->Exists(fullPath)) {
                return fullPath;
            }
        }
    }

    return { };
}

std::unique_ptr<IFileStream> Filesystem::LoadAsIO(const std::string_view uri) const {
    return m_backend->OpenRead(ResolvePath(uri));
}

std::string Filesystem::LoadAsString(const std::string_view uri) const {
    const std::unique_ptr<IFileStream> stream = LoadAsIO(uri);
    if (!stream) return { };

    const std::size_t size = stream->Size();
    if (size == 0) return { };

    std::string result;
    result.resize(size);
    stream->Read(result.data(), size);
    return result;
}

std::vector<std::byte> Filesystem::LoadAsVectorByte(const std::string_view uri) const {
    return LoadAsGenericVector<std::byte>(*this, uri);
}

std::vector<std::uint8_t> Filesystem::LoadAsVector8(const std::string_view uri) const {
    return LoadAsGenericVector<std::uint8_t>(*this, uri);
}

std::vector<std::uint32_t> Filesystem::LoadAsVector32(const std::string_view uri) const {
    return LoadAsGenericVector<std::uint32_t>(*this, uri);
}
}
