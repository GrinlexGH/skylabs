#ifdef _WIN32
#error
#else

#include <dlfcn.h>
#include <cstdlib>

#include "console.hpp"
#include "utilities.hpp"

void *LoadLib(std::string path) {
    Msg("Loading library...\n");
    Msg("library to add: %s\n", path.data());

    void *lib = dlopen(path.data(), RTLD_NOW);
    if (!lib) {
        throw std::runtime_error(std::string("failed open library:\n\n") +
                             dlerror());
    }

    Msg("Library loaded.\n\n");
    return lib;
}

#endif
