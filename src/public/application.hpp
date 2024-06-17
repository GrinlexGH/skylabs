#pragma once

#include <filesystem>
#include <string>

class Application {
  static bool debugMode;

public:
  static void Init();
  static void AddLibSearchPath(const std::string_view path);
  static void *LoadLib(std::string path);
  static void switchDebugMode();
  static bool isDebugMode();
  static std::filesystem::path rootDir;

protected:
  // Singleton stuff
  Application(const Application &) = delete;
  Application(Application &&) = delete;
  Application &operator=(const Application &) = delete;
  Application &operator=(Application &&) = delete;
};
