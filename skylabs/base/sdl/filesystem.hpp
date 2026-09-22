#pragma once
#include <SDL3/SDL_iostream.h>

#include "skylabs/base/filesystem.hpp"

namespace sk::sdl {
class SK_BASE_PUBLIC_CLASS FileStream final : public filesystem::IFileStream {
public:
    explicit FileStream(SDL_IOStream* stream);
    FileStream(const FileStream&) = delete;
    FileStream(FileStream&& rhs) noexcept;
    FileStream& operator=(const FileStream&) = delete;
    FileStream& operator=(FileStream&& rhs) noexcept;
    ~FileStream() override;

    std::size_t Read(void* ptr, std::size_t size) override;
    std::size_t Write(const void* ptr, std::size_t size) override;
    std::int64_t Seek(std::int64_t offset, filesystem::Whence whence) override;
    std::int64_t Tell() override;
    std::size_t Size() override;
    bool Flush() override;

    [[nodiscard]] static SDL_IOStream* CreateIOStreamFromResource(IFileStream* stream);

private:
    void Close();

    SDL_IOStream* m_stream = nullptr;
};

class SK_BASE_PUBLIC_CLASS FilesystemBackend final : public filesystem::IFilesystemBackend {
public:
    ~FilesystemBackend() = default;

    [[nodiscard]] bool Exists(const std::string& path) const override;
    [[nodiscard]] std::unique_ptr<filesystem::IFileStream> OpenRead(
        const std::string& path) const override;
};
}
