#include <filesystem>
#include <iostream>
#include "exception.hpp"
#include "charconverters.hpp"
#include "baseapplication.hpp"

std::filesystem::path BaseApplication::rootDir;

void BaseApplication::Init() {
    try {
        rootDir = std::filesystem::current_path();
    }
    catch(std::filesystem::filesystem_error const& ex) {
        std::cerr << ex.what();
    }
}

void BaseApplication::AddToEnvPATH(const std::string_view path) {
    // Getting length of PATH
    size_t envPathLen = 0;

    // First call to get the length of PATH
    if (!getenv_s(&envPathLen, nullptr, 0, "PATH")) {
        throw Exception("Failed to get PATH length прикол");
    }

    if (envPathLen > 0) {
        std::string envPath(envPathLen, '\0');

        // Second call to actually retrieve the PATH value
        if (!getenv_s(&envPathLen, envPath.data(), envPathLen, "PATH")) {
            throw Exception("Failed to get PATH");
        }

        // Construct the new PATH value
        std::string newEnvPath = "PATH=";
        newEnvPath.append(envPath).append(path.empty() ? "" : ";").append(path);

        // Set the new PATH value
        if (!_putenv(newEnvPath.c_str())) {
            throw Exception("Failed to set PATH");
        }
    }
    else {
        throw Exception("Failed to find PATH");
    }
}

