# About
This is my sandbox, where I test some c++ features, project architectures and much more.

Abstract goal is to make some kind of 3D game from scratch on c++.

## Build
Requirements:
* `CMake`
* `Ninja`
* `MSVC` / `g++` (>=13) / `clang++` (>=15)
* `Python` (>=3.8)
* [`Vulkan SDK`](https://vulkan.lunarg.com/sdk/home)

CMake uses `build_dependencies` python script to install all third-party libraries.
It skips rebuilds using Git submodule commit hashes and can install header only libraries.
You can specify cmake arguments for all submodules or for a specific one by defining DEPS_CMAKE_ARGS and DEPS_CMAKE_LIB_ARGS cmake variables. See `build_dependencies.py --help`.

**You need to install the dependencies to build these libraries yourself**, but usually everything should work without problems.
The largest library that requires much dependencies is **SDL**. If you're on linux, check out [**this file**](libs/sources/SDL/docs/README-linux.md) to compile it.

*If you have any problems with compilation, please write to issues.*

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Binaries will be located in the `.output` folder


Discord: [grinlex](https://discord.gg/WX9HSAmPDF)

## Cloning
This repository contains submodules for external dependencies, so when doing a fresh clone you need to clone recursively:

```bash
git clone --recursive https://github.com/GrinlexGH/skylabs.git
```

Existing repositories can be updated manually:

```bash
git submodule update --init --recursive
```

---
Useful links:
- https://utf8everywhere.org/
- https://github.com/boostorg/dll/blob/e7ad58bfb91bd8a79e0ba274f80ae6a8da9fc59e/include/boost/dll/detail/posix/program_location_impl.hpp#L25
- https://github.com/KhronosGroup/Vulkan-Samples/tree/main
