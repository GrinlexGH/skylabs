#pragma once

#include <filesystem>
#include <string>

class BaseApplication
{
protected:
public:
    static void Init();
    // input -- path to folder in format like C:/Program Files(x86)/Steam/Half-Life 2/bin
    static void AddToEnvPATH(const std::string_view path);
    static std::filesystem::path rootDir;
};

