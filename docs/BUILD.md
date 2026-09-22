# Build Guide

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [One-Time Setup](#one-time-setup)
3. [Compiler Profiles](#compiler-profiles)
4. [Building Per Toolchain](#building-per-toolchain)
    - [MSVC](#msvc)
    - [Clang(-cl)](#clang-cl)
    - [MinGW](#mingw)
    - [Linux](#linux)
5. [Missing `.pdb` Files](#missing-pdb-files)
6. [Android](#android)


## Prerequisites

| Tool             | Requirement                                                      |
|------------------|------------------------------------------------------------------|
| **CMake**        | Latest version                                                   |
| **Ninja**        | Latest version                                                   |
| **C++ Compiler** | Windows MSVC / Clang / MinGW (GCC or Clang) / Linux GCC or Clang |
| **Conan**        | 2.x, installed via `pip`, which always will be the latest        |

> [!TIP]
> It is highly recommended to check out [Scoop](https://scoop.sh/), an incredibly convenient package manager for Windows.

You can use your package manager to install [CMake](https://cmake.org/download/) and [Ninja](https://github.com/ninja-build/ninja/releases).

On Windows, it is recommended to use [Visual Studio](https://visualstudio.microsoft.com/) with [LLVM/Clang](https://github.com/llvm/llvm-project/releases).

Latest version of Conan is available in pip:

```bash
uv venv

# Windows
.venv\Scripts\activate
# Linux
source .venv/bin/activate

uv pip install -U -r ./requirements.txt
```

## One-Time Setup

Run these once per machine (the recipe index / config steps again any time they change structure).

---

**Generate the default profile:**

```bash
conan profile detect --force
```

It's required even though you'll pass a custom profile via `-pr`, because Conan 2 still needs a "default" profile for the *build* context (tools that run on the build machine) unless you also pass `-pr:b` explicitly.

---

**Register the local recipe index**:

```bash
conan remote add skylabs ./conan/conan-recipes -t local-recipes-index -f
```

The `skylabs` remote must come **before** `conancenter` in every `conan install` so packages with overridden recipes resolve to the local, patched versions rather than the upstream ones.

---

**Install Conan config:**

> [!CAUTION]
> `conan config install` **overwrites your global Conan configuration**. If you use Conan for
> other projects on the same machine, back up `~/.conan2` first.

```bash
# Install the global Conan config (profiles, hooks, settings)
conan config install ./conan/conan-config/config
```

## Compiler Profiles

| Profile         | Toolchain                           | Runtime   | Notes                                                                          |
|-----------------|-------------------------------------|-----------|--------------------------------------------------------------------------------|
| `msvc-18`       | MSVC                                | MSVC STL  | Default on Windows                                                             |
| `clang-llvm`    | LLVM's clang (standalone install)   | MSVC STL  | Better codegen than MSVC                                                       |
| `clang-cl-18`   | Clang-cl bundled with Visual Studio | MSVC STL  | Integrates poorly even with its native Visual Studio. It exists because it can |
| `clang-clang64` | MSYS2 Clang, `clang64` environment  | libc++    | MinGW-flavored, for fun or experements                                         |
| `gcc-ucrt64`    | MSYS2 GCC, `ucrt64` environment     | libstdc++ | MinGW-flavored, for fun or experements                                         |
| `clang-linux`   | Clang                               | libc++    |                                                                                |
| `gcc-linux`     | GCC                                 | libstdc++ |                                                                                |

Rule of thumb for picking one:

- **Windows, want the "correct" ABI:** `msvc-18` or `clang-llvm` - both link the MSVC STL, so third-party binaries and debug info behave consistently.
- **Windows, just experimenting:** `clang-clang64` or `gcc-ucrt64`. Expect friction - see [MinGW](#mingw) below.
- **Linux:** I find Clang more appealing.

## Building Per Toolchain

All commands below assume you're in the repository root and have already done the [one-time setup](#one-time-setup).

### MSVC

```bash
conan install . -pr msvc-18 -r skylabs -r conancenter -s build_type=Debug -s compiler.runtime_type=Debug --build=missing
```

`compiler.runtime_type` (Debug/Release) must match across *every* dependency and the main project.
Mixing them triggers an `_ITERATOR_DEBUG_LEVEL` mismatch at link time for static libraries.
The STL container ABI literally changes between debug and release runtimes.

MSVC has an amazing debugger in Visual Studio, but terrible IntelliSense and code generation.

### Clang(-cl)

```bash
# Standalone LLVM/clang
conan install . -pr clang-llvm -r skylabs -r conancenter -s build_type=Debug -s compiler.runtime_type=Debug --build=missing

# Clang-cl bundled with Visual Studio
conan install . -pr clang-cl-18 -r skylabs -r conancenter -s build_type=Debug -s compiler.runtime_type=Debug --build=missing
```

Recommended way to build on Windows is `clang-llvm` profile.

Both variants still link the **MSVC STL**, so the `_ITERATOR_DEBUG_LEVEL` rule above applies here too.

Using the built-in clang-cl in Visual Studio is not recommended, as it has poor support even within Visual Studio itself.
Furthermore, the built-in version always lags behind the latest release.

Differences between Clang flavors on Windows are described [here](https://blog.conan.io/2022/10/13/Different-flavors-Clang-compiler-Windows.html).

### MinGW

```bash
# Clang in the clang64 MSYS2 environment
conan install . -pr clang-clang64 -r skylabs -r conancenter -s build_type=Debug --build=missing

# GCC in the ucrt64 MSYS2 environment
conan install . -pr gcc-ucrt64 -r skylabs -r conancenter -s build_type=Debug --build=missing
```

Binaries built with MinGW cannot be mixed with binaries built with MSVC STL.
Don't try to link a MinGW-built dependency into an MSVC-built target, or vice versa (it definitely doesn't work for non-flat Steamworks SDK part).

And really, MinGW is just a collection of kludges I guess.

### Linux

```bash
conan install . -pr clang-linux -r skylabs -r conancenter -s build_type=Debug --build=missing

conan install . -pr gcc-linux -r skylabs -r conancenter -s build_type=Debug --build=missing
```

Make sure every dependency in the graph linked with `libstdc++` only or `libc++` only.
If you want `libstdc++` with `clang-linux` profile, just type `-s compiler.libcxx=libstdc++11` at the end of the `conan install` command.

After any of the above, finish the build the same way regardless of toolchain:

```bash
cmake --preset conan-debug # or conan-default
cmake --build build
```

## Missing `.pdb` Files

Some upstream recipes don't package their `.pdb` files, which makes MSVC warn on every link.
The project's Conan config installs a hook [`hook_copy_pdbs_to_package.py`](https://docs.conan.io/2/examples/dev_flow/debug/debugging_visual.html#installing-a-hook-to-copy-the-pdbs-to-the-package-folder), that copies missing `.pdb`s into the package folder automatically after the build step.
This  hook is installed as part of [`conan config install`](#one-time-setup).

# Android

The best way to build an Android project is to use Android Studio.

That is certainly what I use. In principle, a standard Android SDK and NDK (downloaded manually) combined with command-line Gradle should work, but I haven't verified this myself.

You shouldn't run into any build issue, just follow the instructions in Android Studio. Everything there is intuitive I guess.
