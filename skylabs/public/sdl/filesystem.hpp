#pragma once
#include <skylabs/public/filesystem.hpp>

namespace SDL {
class PUBLIC_CLASS CFileStream final : public IFileStream {
public:
    explicit CFileStream(SDL_IOStream* stream);
    CFileStream(const CFileStream&) = delete;
    CFileStream(CFileStream&& rhs) noexcept;
    CFileStream& operator=(const CFileStream&) = delete;
    CFileStream& operator=(CFileStream&& rhs) noexcept;
    ~CFileStream() override;

    std::size_t Read(void* ptr, std::size_t size) override;
    std::size_t Write(const void* ptr, std::size_t size) override;
    std::int64_t Seek(std::int64_t offset, Whence whence) override;
    std::int64_t Tell() override;
    std::size_t Size() override;
    bool Flush() override;

private:
    void Close();

    SDL_IOStream* m_stream = nullptr;
};

[[nodiscard]] PUBLIC_CLASS SDL_IOStream* CreateIOStreamFromResource(IFileStream* stream);

class PUBLIC_CLASS CFilesystemBackend final : public IFilesystemBackend
{
public:
    ~CFilesystemBackend() = default;

    [[nodiscard]] bool Exists(const std::string& path) const override;
    [[nodiscard]] std::unique_ptr<IFileStream> OpenRead(const std::string& path) const override;
};
}
