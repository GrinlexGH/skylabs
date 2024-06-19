#ifdef _WIN32
#error
#else
#include <dlfcn.h>
#include "console.hpp"
#include "utilities.hpp"

void AddLibSearchPath(const std::string_view path) {
    Msg("Adding library serach path...\n");
    if (path.empty()) {
        Msg("Library search not path added.\n\n");
        return;
    }
    Msg("Path to add: %s\n", path.data());

    std::string newPath;
    if (newPath = getenv("LD_LIBRARY_PATH"); newPath.empty()) 
        newPath += ':';
    newPath = path.data();

    setenv("LD_LIBRARY_PATH", newPath.c_str(), 1);
    Msg("Library search path added.\n\n");
}

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
