# About
This is my trying to make some kind of 3D game from scratch on c++. I know I will write it all my life because I use vulkan, but anyway I like it.

## Build
Requirements:
* CMake
* [Ninja](https://ninja-build.org/)
* `MSVC` / `g++` (>=13) / `clang++` (>=15)
* [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

CMake uses `build_dependencies` script to build all third-party libraries. It outputs the compiled libraries into the `libs/bin/<platform>` directory. It uses the `Ninja` generator and assumes the compiler is either **GCC** or **Clang**.

On Windows, the build output path and generator depend on the selected compiler:

- If **MSVC** is used, libraries are built into `libs/bin/windows/static/msvc` using the default CMake generator, assuming compiler is **MSVC** .

- If **MinGW** or **LLVM** is used, libraries are built into `libs/bin/windows/static/mingw` using the `Ninja` generator, assuming the compiler is either **GCC** or **Clang**.

This separation is necessary because static library formats differ between **MSVC** and **GCC**/**Clang**.

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
