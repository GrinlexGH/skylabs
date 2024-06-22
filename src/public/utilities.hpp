#include "platform.hpp"
#include <string>

// System depended
PLATFORM_CLASS void AddLibSearchPath(const std::string_view path);
PLATFORM_CLASS void *LoadLib(std::string path);
