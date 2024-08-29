#pragma once
#include "platform.hpp"

#include <vector>
#include <string_view>

namespace resource_loader {

    // \param filename UTF-8 encoded path to file from the application's root
    PLATFORM_CLASS std::vector<char> ReadFile(const std::string_view filename);

} // resource_loader
