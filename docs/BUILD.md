# Build Guide

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [One-Time Setup](#one-time-setup)
3. [Compiler Profile Matrix](#compiler-profile-matrix)
4. [Building Per Toolchain](#building-per-toolchain)
    - [MSVC](#msvc)
    - [Clang-cl](#clang-cl)
    - [MinGW](#mingw)
    - [Linux](#linux)
5. [Missing `.pdb` Files](#missing-pdb-files)
6. [Troubleshooting](#troubleshooting)

## Prerequisites

| Tool             | Requirement                                                    |
|------------------|----------------------------------------------------------------|
| **CMake**        | Latest version                                                 |
| **Ninja**        | Latest version                                                 |
| **C++ Compiler** | MSVC / Clang-cl / MinGW (GCC or Clang) / Linux GCC or Clang    |
| **Conan**        | 2.x, installed via `pip`/`uv`, which always will be the latest |
| **Android SDK**  | Optional, only for Android builds                              |

Install Conan in a virtual environment so it doesn't collide with other Python tooling:

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
| `msvc-18`       | MSVC                                | MSVC STL  | Default recommendation on Windows                                              |
| `clang-llvm`    | LLVM's clang (standalone install)   | MSVC STL  | Better codegen than VS's bundled clang-cl                                      |
| `clang-cl-18`   | Clang-cl bundled with Visual Studio | MSVC STL  | Integrates poorly even with its native Visual Studio. It exists because it can |
| `clang-clang64` | MSYS2 Clang, `clang64` environment  | libc++    | MinGW-flavored, for fun or experements                                         |
| `gcc-ucrt64`    | MSYS2 GCC, `ucrt64` environment     | libstdc++ | MinGW-flavored, for fun or experements                                         |
| `clang-linux`   | Clang                               | libc++    |                                                                                |
| `gcc-linux`     | GCC                                 | libstdc++ |                                                                                |

Rule of thumb for picking one:

- **Windows, want the "correct" ABI:** `msvc-18` or `clang-llvm` - both link the MSVC STL, so third-party binaries and debug info behave consistently.
- **Windows, just experimenting / MinGW-only workflow:** `clang-clang64` or `gcc-ucrt64`. Expect friction - see [MinGW](#mingw) below.
- **Linux:** pick whichever matches the standard library your other dependencies expect.

## Building Per Toolchain

All commands below assume you're in the repository root and have already done the
[one-time setup](#one-time-setup).

### MSVC

```bash
conan install . -pr msvc-18 -r skylabs -r conancenter -s build_type=Debug -s compiler.runtime_type=Debug --build=missing
```

**Nuance:** `compiler.runtime_type` (Debug/Release) must match across *every* dependency and the main project.
Mixing them triggers an `_ITERATOR_DEBUG_LEVEL` mismatch at link time for static libraries.
The STL container ABI literally changes between debug and release runtimes.

### Clang(-cl)

```bash
# Standalone LLVM/clang
conan install . -pr clang-llvm -r skylabs -r conancenter -s build_type=Debug -s compiler.runtime_type=Debug --build=missing

# Clang-cl bundled with Visual Studio
conan install . -pr clang-cl-18 -r skylabs -r conancenter -s build_type=Debug -s compiler.runtime_type=Debug --build=missing
```

**Nuance:** both variants still link the **MSVC STL**, so the `_ITERATOR_DEBUG_LEVEL` rule above applies here too.
The two variants differ in optimizer maturity and how current the Clang version is.
LLVM's standalone releases usually ship newer Clang than the copy bundled with a given VS version.
Differences between Clang flavors on Windows are described [here](https://blog.conan.io/2022/10/13/Different-flavors-Clang-compiler-Windows.html).

### MinGW

```bash
# Clang in the clang64 MSYS2 environment
conan install . -pr clang-clang64 -r skylabs -r conancenter -s build_type=Debug --build=missing

# GCC in the ucrt64 MSYS2 environment
conan install . -pr gcc-ucrt64 -r skylabs -r conancenter -s build_type=Debug --build=missing
```

**Nuances:**
- **ABI incompatibility with MSVC:** binaries built with MinGW cannot be mixed with binaries built with MSVC STL.
  Don't try to link a MinGW-built dependency into an MSVC-built target, or vice versa.
- Some upstream ConanCenter recipes special-case MinGW poorly (e.g. Boost's `b2` build system misidentifying MinGW's Clang as `clang-cl` and picking the wrong flags).
  This is one of the reasons the project carries its own recipe overrides for those libraries.

### Linux

```bash
conan install . -pr clang-linux -r skylabs -r conancenter -s build_type=Debug --build=missing

conan install . -pr gcc-linux -r skylabs -r conancenter -s build_type=Debug --build=missing
```

**Nuance:** make sure every dependency in the graph linked with `libstdc++` only or `libc++` only.
If you want `libstdc++` with `clang-linux` profile, just type `-s compiler.libcxx=libstdc++` at the end of the command line.

After any of the above, finish the build the same way regardless of toolchain:

```bash
cmake --preset conan-default # or conan-debug
cmake --build build
```

## Missing `.pdb` Files

Some upstream recipes don't package their `.pdb` files, which makes MSVC warn on every link.
The project's Conan config installs a hook [`hook_copy_pdbs_to_package.py`](https://docs.conan.io/2/examples/dev_flow/debug/debugging_visual.html#installing-a-hook-to-copy-the-pdbs-to-the-package-folder), that copies missing `.pdb`s into the package folder automatically after the build step.
This  hook is installed as part of [`conan config install`](#one-time-setup).
