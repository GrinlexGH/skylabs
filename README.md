# About
This is my sandbox, where I test some c++ features, project architectures and much more.

Abstract goal is to make some kind of 3D game from scratch on c++.

## Build
Requirements:
* `CMake`
* `MSVC` or `g++` (>=13) or `clang++` (>=15)
* `Python` (>=3.8)
* [`Vulkan SDK`](https://vulkan.lunarg.com/sdk/home)

CMake uses python script to install all third-party libraries.

For more info see [**this repository**](https://github.com/GrinlexGH/deps).

Set `DEPS_TARGET_SYSTEM`, `DEPS_TARGET_ARCH` and `DEPS_SUBDIR` variables or env variables to control `DEPS_INSTALL_DIR` path (e.g. `third_party/bin/Linux-x86_64/libcxx/`) - useful for cross-compilation via `CMAKE_TOOLCHAIN_FILE`, or for [`cmake-tools-kits.json`](https://gist.github.com/GrinlexGH/cffbe9727b7183d7044e2c4af378ffd2) from vscode extension.


**You need to install the dependencies to build these libraries yourself**, but usually everything should work without problems.
The largest library that requires much dependencies is **SDL**. If you're on linux, check out [**this file**](libs/sources/SDL/docs/README-linux.md) to compile it.

*If you have any problems with compilation, please write to issues.*

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Binaries will be located in the `.output` folder.


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
