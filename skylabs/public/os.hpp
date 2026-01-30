#pragma once
#include <skylabs/public/dll_export.hpp>

#include <filesystem>

namespace OS {
PUBLIC_CLASS std::filesystem::path GetExecutableDirectory();
}
