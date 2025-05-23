#include "os.hpp"

#include <filesystem>

#include "logging.hpp"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <conio.h>
#include <Shlobj.h>
#elif defined(PLATFORM_UNIX)
#include <mutex>
#include <termios.h>
#include <unistd.h>
#endif

namespace {
#ifdef PLATFORM_WINDOWS
std::wstring Widen(const std::string_view narrowStr) {
    if (narrowStr.empty()) {
        return {};
    }

    const int len = MultiByteToWideChar(
        CP_UTF8, 0,
        narrowStr.data(), static_cast<int>(narrowStr.size()),
        nullptr, 0);

    std::wstring out(len, 0);
    MultiByteToWideChar(
        CP_UTF8, 0,
        narrowStr.data(), static_cast<int>(narrowStr.size()),
        out.data(), len);

    return out;
}
#elif defined(PLATFORM_UNIX)
//====================
// Enables non-canonical input for terminal in its scope
// Ensures thread-safe terminal mode synchronization via RAII-scoped lock
//====================
class CCanonicalMode
{
public:
    CCanonicalMode() = delete;
    CCanonicalMode(const CCanonicalMode&) = delete;
    CCanonicalMode(CCanonicalMode&&) = delete;
    CCanonicalMode& operator=(const CCanonicalMode&) = delete;
    CCanonicalMode& operator=(CCanonicalMode&&) = delete;

    explicit CCanonicalMode(const bool echo) : m_termLock(m_termMutex) {
        tcgetattr(0, &m_old);
        m_current = m_old;
        m_current.c_lflag &= ~(ICANON | ECHO);
        if (echo) {
            m_current.c_lflag |= ECHO;
        }
        tcsetattr(0, TCSANOW, &m_current);
    }

    ~CCanonicalMode() {
        tcsetattr(0, TCSANOW, &m_old);
    }

private:
    static inline termios m_old {}, m_current {};
    static inline std::mutex m_termMutex {};
    std::lock_guard<std::mutex> m_termLock;
};
#endif
}

namespace OS {
#ifdef PLATFORM_WINDOWS
void WaitAnyKey() {
    _getch();
}
#elif defined(PLATFORM_UNIX)
void WaitAnyKey() {
    CCanonicalMode enable(false);
    getchar();
}
#endif
}
