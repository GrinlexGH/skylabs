#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "sk_base_export.h"

namespace sk::filesystem {
enum class Whence : std::uint8_t {
    eBegin,
    eCursor,
    eEnd,
};

class SK_BASE_PUBLIC_CLASS IFileStream {
public:
    virtual ~IFileStream() = default;
    virtual std::size_t Read(void* ptr, std::size_t size) = 0;
    virtual std::size_t Write(const void* ptr, std::size_t size) = 0;
    virtual std::int64_t Seek(std::int64_t offset, Whence whence) = 0;
    virtual std::int64_t Tell() = 0;
    virtual std::size_t Size() = 0;
    virtual bool Flush() = 0;
};

class SK_BASE_PUBLIC_CLASS IFilesystemBackend {
public:
    virtual ~IFilesystemBackend() = default;
    [[nodiscard]] virtual bool Exists(const std::string& path) const = 0;
    [[nodiscard]] virtual std::unique_ptr<IFileStream> OpenRead(const std::string& path) const = 0;
};

class SK_BASE_PUBLIC_CLASS Filesystem {
public:
    explicit Filesystem(std::nullptr_t) { }
    explicit Filesystem(std::unique_ptr<IFilesystemBackend> backend);

    void Mount(std::string_view scheme, std::string_view physicalPath);
    [[nodiscard]] std::string ResolvePath(std::string_view uri) const;

    [[nodiscard]] std::string LoadAsString(std::string_view uri) const;
    [[nodiscard]] std::unique_ptr<IFileStream> LoadAsIO(std::string_view uri) const;
    [[nodiscard]] std::vector<std::byte> LoadAsVectorByte(std::string_view uri) const;
    [[nodiscard]] std::vector<std::uint8_t> LoadAsVector8(std::string_view uri) const;
    [[nodiscard]] std::vector<std::uint32_t> LoadAsVector32(std::string_view uri) const;

private:
    std::unique_ptr<IFilesystemBackend> m_backend;
    std::unordered_map<std::string, std::vector<std::string>> m_mountPoints;
};
}
