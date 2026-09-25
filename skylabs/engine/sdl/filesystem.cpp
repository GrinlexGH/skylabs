#include "skylabs/engine/sdl/filesystem.hpp"

namespace {
Sint64 SDLCALL StreamSizeBridge(void* userdata) {
    return static_cast<Sint64>(static_cast<sk::filesystem::IFileStream*>(userdata)->Size());
}

Sint64 SDLCALL StreamSeekBridge(void* userdata, const Sint64 offset, const SDL_IOWhence whence) {
    auto* stream = static_cast<sk::filesystem::IFileStream*>(userdata);
    auto w = sk::filesystem::Whence::eBegin;
    if (whence == SDL_IO_SEEK_CUR)
        w = sk::filesystem::Whence::eCursor;
    else if (whence == SDL_IO_SEEK_END)
        w = sk::filesystem::Whence::eEnd;

    return static_cast<Sint64>(stream->Seek(offset, w));
}

std::size_t SDLCALL StreamReadBridge(void* userdata, void* ptr, const std::size_t size,
                                     SDL_IOStatus* status) {
    auto* stream = static_cast<sk::filesystem::IFileStream*>(userdata);
    const std::size_t read = stream->Read(ptr, size);
    if (status) *status = (read == 0 && size > 0) ? SDL_IO_STATUS_EOF : SDL_IO_STATUS_READY;
    return read;
}

std::size_t SDLCALL StreamWriteBridge(void* userdata, const void* ptr, const size_t size,
                                      SDL_IOStatus* status) {
    auto* stream = static_cast<sk::filesystem::IFileStream*>(userdata);
    const std::size_t written = stream->Write(ptr, size);
    if (status) *status = (written < size) ? SDL_IO_STATUS_ERROR : SDL_IO_STATUS_READY;
    return written;
}

bool SDLCALL StreamFlushBridge(void* userdata, SDL_IOStatus* status) {
    const bool ok = static_cast<sk::filesystem::IFileStream*>(userdata)->Flush();
    if (status) *status = ok ? SDL_IO_STATUS_READY : SDL_IO_STATUS_ERROR;
    return ok;
}

bool SDLCALL StreamCloseBridge(void* /*userdata*/) { return true; }
}

namespace sk::sdl {
FileStream::FileStream(SDL_IOStream* stream) : m_stream(stream) { }

FileStream::FileStream(FileStream&& rhs) noexcept : m_stream(std::exchange(rhs.m_stream, nullptr)) { }

FileStream& FileStream::operator=(FileStream&& rhs) noexcept {
    if (this != &rhs) {
        Close();
        m_stream = std::exchange(rhs.m_stream, nullptr);
    }
    return *this;
}

FileStream::~FileStream() { Close(); }

std::size_t FileStream::Read(void* ptr, const std::size_t size) {
    return m_stream ? SDL_ReadIO(m_stream, ptr, size) : 0;
}

std::size_t FileStream::Write(const void* ptr, const std::size_t size) {
    return m_stream ? SDL_WriteIO(m_stream, ptr, size) : 0;
}

std::int64_t FileStream::Seek(const std::int64_t offset, filesystem::Whence whence) {
    if (!m_stream) return -1;

    const SDL_IOWhence sdlWhence = [whence] {
        switch (whence) {
            case filesystem::Whence::eBegin:
                return SDL_IO_SEEK_SET;
            case filesystem::Whence::eCursor:
                return SDL_IO_SEEK_CUR;
            case filesystem::Whence::eEnd:
                return SDL_IO_SEEK_END;
        }
        std::unreachable();
    }();

    return SDL_SeekIO(m_stream, offset, sdlWhence);
}

std::int64_t FileStream::Tell() { return m_stream ? SDL_TellIO(m_stream) : -1; }

std::size_t FileStream::Size() {
    return m_stream ? static_cast<std::size_t>(SDL_GetIOSize(m_stream)) : 0;
}

bool FileStream::Flush() { return m_stream ? SDL_FlushIO(m_stream) : false; }

SDL_IOStream* FileStream::CreateIOStreamFromResource(IFileStream* stream) {
    if (!stream) return nullptr;

    static SDL_IOStreamInterface iface;
    SDL_INIT_INTERFACE(&iface);
    iface.size = StreamSizeBridge;
    iface.seek = StreamSeekBridge;
    iface.read = StreamReadBridge;
    iface.write = StreamWriteBridge;
    iface.flush = StreamFlushBridge;
    iface.close = StreamCloseBridge;

    return SDL_OpenIO(&iface, stream);
}

void FileStream::Close() {
    if (m_stream) {
        SDL_CloseIO(m_stream);
        m_stream = nullptr;
    }
}

bool FilesystemBackend::Exists(const std::string& path) const {
    SDL_PathInfo info;
    return SDL_GetPathInfo(path.c_str(), &info);
}

std::unique_ptr<filesystem::IFileStream> FilesystemBackend::OpenRead(const std::string& path) const {
    auto* handle = SDL_IOFromFile(path.c_str(), "rb");
    if (!handle) return nullptr;
    return std::make_unique<FileStream>(handle);
}
}
