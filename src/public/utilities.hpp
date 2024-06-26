#include "platform.hpp"
#include <string>

// System depended
#ifdef _WIN32
PLATFORM_CLASS void AddLibSearchPath(const std::string_view path);
#endif

PLATFORM_CLASS void *LoadLib(std::string path);
