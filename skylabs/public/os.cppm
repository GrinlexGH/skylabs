module;
#include <skylabs/public/dll_export.hpp>
export module skylabs.pub.os;
export import std;

export namespace OS {
PUBLIC_CLASS std::string GetExecutableDirectory();

template <typename... Args>
[[nodiscard]] std::string PathJoin(Args&&... args) {
    std::filesystem::path result;
    ((result /= std::forward<Args>(args)), ...);
    return result.string();
}
}
