# About
This is my own game engine and I plan to include all the best that other game engines have in it.

Requirements for build:
* CMake
* Ninja
* MSVC, g++/clang++
* Vulkan SDK

For other libraries cmake uses `build_dependencies` script. **You must install all dependencies to build these libraries manually.**

Discord: [grinlex](https://discord.gg/WX9HSAmPDF)

## Cloning
This repository contains submodules for external dependencies, so when doing a fresh clone you need to clone recursively:

```
git clone --recursive https://github.com/GrinlexGH/skylabs.git
```

Existing repositories can be updated manually:

```
git submodule init
git submodule update
```

---
Useful links:
- https://utf8everywhere.org/
- https://github.com/boostorg/dll/blob/e7ad58bfb91bd8a79e0ba274f80ae6a8da9fc59e/include/boost/dll/detail/posix/program_location_impl.hpp#L25
