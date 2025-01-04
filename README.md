# About
This is my own game engine and I plan to include all the best that other game engines have in it.

Requirements for build:
* CMake
* Ninja
* MSVC, g++(at least 13)/clang++(at least 15)
* Vulkan SDK

For other libraries cmake uses `build_dependencies` script. **You must install all dependencies to build these libraries manually.**
You will compile all the dependencies only once, so the first build will be long, but all subsequent builds will be fast if you don't delete the `libs/bin/*platform*` folder.

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
