#include <skylabs/public/sdl/filesystem.hpp>

namespace {
Sint64 SDLCALL StreamSizeBridge(void* userdata) {
    return static_cast<Sint64>(static_cast<IFileStream*>(userdata)->Size());
}

Sint64 SDLCALL StreamSeekBridge(void* userdata, Sint64 offset, SDL_IOWhence whence) {
    auto* stream = static_cast<IFileStream*>(userdata);
    Whence w = Whence::eBegin;
    if (whence == SDL_IO_SEEK_CUR) w = Whence::eCursor;
    else if (whence == SDL_IO_SEEK_END) w = Whence::eEnd;

    return static_cast<Sint64>(stream->Seek(offset, w));
}

std::size_t SDLCALL StreamReadBridge(void* userdata, void* ptr, size_t size, SDL_IOStatus* status) {
    auto* stream = static_cast<IFileStream*>(userdata);
    std::size_t read = stream->Read(ptr, size);
    if (status) *status = (read == 0 && size > 0) ? SDL_IO_STATUS_EOF : SDL_IO_STATUS_READY;
    return read;
}

std::size_t SDLCALL StreamWriteBridge(void* userdata, const void* ptr, size_t size, SDL_IOStatus* status) {
    auto* stream = static_cast<IFileStream*>(userdata);
    std::size_t written = stream->Write(ptr, size);
    if (status) *status = (written < size) ? SDL_IO_STATUS_ERROR : SDL_IO_STATUS_READY;
    return written;
}

bool SDLCALL StreamFlushBridge(void* userdata, SDL_IOStatus* status) {
    bool ok = static_cast<IFileStream*>(userdata)->Flush();
    if (status) *status = ok ? SDL_IO_STATUS_READY : SDL_IO_STATUS_ERROR;
    return ok;
}

bool SDLCALL StreamCloseBridge(void* /*userdata*/) {
    return true;
}
}

namespace SDL {
CFileStream::CFileStream(SDL_IOStream* stream) : m_stream(stream) {}

CFileStream::CFileStream(CFileStream&& rhs) noexcept : m_stream(std::exchange(rhs.m_stream, nullptr)) {}

CFileStream& CFileStream::operator=(CFileStream&& rhs) noexcept {
    if (this != &rhs) {
        Close();
        m_stream = std::exchange(rhs.m_stream, nullptr);
    }
    return *this;
}

CFileStream::~CFileStream() { Close(); }

std::size_t CFileStream::Read(void* ptr, std::size_t size) {
    return m_stream ? SDL_ReadIO(m_stream, ptr, size) : 0;
}

std::size_t CFileStream::Write(const void* ptr, std::size_t size) {
    return m_stream ? SDL_WriteIO(m_stream, ptr, size) : 0;
}

std::int64_t CFileStream::Seek(std::int64_t offset, Whence whence) {
    if (!m_stream)
        return -1;

    const SDL_IOWhence sdlWhence = [whence] {
        switch (whence) {
            case Whence::eBegin: return SDL_IO_SEEK_SET;
            case Whence::eCursor: return SDL_IO_SEEK_CUR;
            case Whence::eEnd: return SDL_IO_SEEK_END;
        }
        std::unreachable();
    }();

    return SDL_SeekIO(m_stream, offset, sdlWhence);
}

std::int64_t CFileStream::Tell() {
    return m_stream ? SDL_TellIO(m_stream) : -1;
}

std::size_t CFileStream::Size() {
    return m_stream ? static_cast<std::size_t>(SDL_GetIOSize(m_stream)) : 0;
}

bool CFileStream::Flush() { return m_stream ? SDL_FlushIO(m_stream) : false; }

void CFileStream::Close() {
    if (m_stream) {
        SDL_CloseIO(m_stream);
        m_stream = nullptr;
    }
}

SDL_IOStream* CreateIOStreamFromResource(IFileStream* stream) {
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

bool CFilesystemBackend::Exists(const std::string& path) const {
    SDL_PathInfo info;
    return SDL_GetPathInfo(path.c_str(), &info);
}

std::unique_ptr<IFileStream> CFilesystemBackend::OpenRead(const std::string& path) const {
    auto* handle = SDL_IOFromFile(path.c_str(), "rb");
    if (!handle) return nullptr;
    return std::make_unique<CFileStream>(handle);
}
}
