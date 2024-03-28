#ifdef WIN32
#error
#else
#include <filesystem>
#include "console.hpp"

void Application::Init() {
    console::Msg("Initializing CBaseApplication...\n");
    rootDir = std::filesystem::canonical("/proc/self/exe");
    console::Msg("rootDir == %s\n", rootDir.string().c_str());
    console::Msg("Initializing finished.\n\n");
}

void Application::AddLibSearchPath(const std::string_view path) {
    console::Msg("Adding library serach path...\n");
    if (path.empty()) {
        console::Msg("Library search not path added.\n\n");
        return;
    }
    console::Msg("Path to add: %s\n", path.data());

    std::string newPath;

    if (const char* currentLibEnv = getenv("LD_LIBRARY_PATH")) {
        newPath = currentLibEnv + ':';
        newPath += std::bit_cast<const char*>(path.data());
    }
    else {
        newPath = std::bit_cast<const char*>(path.data());
    }

    setenv("LD_LIBRARY_PATH", newPath.c_str(), 1);
    console::Msg("Library search path added.\n\n");
}

void* Application::LoadLib(std::string path) {
    console::Msg("Loading library...\n");
    console::Msg("library to add: %s\n", path.data());

    void* lib = dlopen(std::bit_cast<const char*>(path.data()), RTLD_NOW);
    if (!lib) {
        throw std::runtime_error(std::string("failed open library:\n\n") + dlerror());
    }

    console::Msg("\x1B[38;2;64;224;208mLibrary loaded.\x1B[0m\n\n");
    return lib;
}

void console::Destroy() {
    argv.clear();
}
#endif

