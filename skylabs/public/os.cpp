module;
#include <skylabs/public/dll_export.hpp>
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <boost/nowide/convert.hpp>
#endif
module skylabs.pub.os;

namespace OS {
PUBLIC_CLASS std::string GetExecutableDirectory() {
    static const std::string cachedPath = [] {
        std::filesystem::path p;
#ifdef PLATFORM_WINDOWS
        std::wstring buffer(100, L'\0');
        DWORD size;
        while (true) {
            size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (size < buffer.size())
                break;
            buffer.resize(buffer.size() + 100);
        }
        p = buffer;
        p = p.parent_path();
        return boost::nowide::narrow(p.wstring());
#else
        p = std::filesystem::canonical("/proc/self/exe").parent_path();
        return p.string();
#endif
    }();

    return cachedPath;
}
}
