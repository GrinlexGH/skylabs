#pragma once
#include "publicapi.hpp"

#include <vector>
#include <string_view>

namespace resource_loader {
    PUBLIC_CLASS std::vector<char> ReadFile(std::string_view filename);
}
