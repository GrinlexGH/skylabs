# 🥼 Skylabs

[![Discord](https://img.shields.io/badge/Discord-5865F2?logo=discord&logoColor=white)](https://discord.gg/YqTKmA5qbf)
[![C++](https://img.shields.io/badge/C++-23-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![Vulkan](https://img.shields.io/badge/Vulkan-SDK-red.svg?style=flat&logo=vulkan)](https://vulkan.lunarg.com/)

> [!NOTE]
> **About this project**
>
> This is my personal sandbox for testing modern C++ features, exploring project architectures, and experimenting with graphics programming.
>
> 🎯 **Abstract Goal:** To build a functional 3D game engine from scratch using C++.

## 📥 Cloning the Repository

This project uses **Git Submodules**. It is crucial to include them during the clone process.

### ✅ Correct way to clone
Use the `--recursive` flag to ensure all dependencies are downloaded:

```bash
git clone --recursive https://github.com/GrinlexGH/skylabs.git
````

### ⚠️ If you already cloned (without recursive)

If the folder is already on your disk but submodule folders are empty, run this command in the project root:

```bash
git submodule update --init --recursive
```

## 🛠️ Build Requirements

Ensure you have the following tools installed before compiling:

| Tool             | Requirement               |
|------------------|---------------------------|
| **CMake**        | Latest version            |
| **Ninja**        | Latest version            |
| **C++ Compiler** | Latest MSVC / GCC / Clang |
| **Conan**        | Latest version            |

## ⚙️ Configuration & Building

### 🦐 0. Install requirements

> [!TIP]
> It is highly recommended to check out [Scoop](https://scoop.sh/), an incredibly convenient package manager for Windows.

You can use your package manager to install [CMake](https://cmake.org/download/) and [Ninja](https://github.com/ninja-build/ninja/releases).

On Windows, it is recommended to use [Visual Studio](https://visualstudio.microsoft.com/) with [LLVM/Clang](https://github.com/llvm/llvm-project/releases).

Conan is a pip package, so you need to install it either in a Python `venv` environment or globally using a package manager, if the latest version is available:
```bash
uv venv

# For Windows
.venv\Scripts\activate
# For Linux
source .venv/bin/activate

uv pip install -U -r ./requirements.txt
```

### 🔧 1. Configure Conan

This project uses a custom Conan recipe index.

**Add the local remote:**

```bash
conan remote add skylabs ./conan/conan-recipes -t local-recipes-index -f
```

> [!TIP]
> The `skylabs` remote is required to resolve internal packages used by the project.

---

### 🌍 Install Global Conan Config

> [!CAUTION]
> This will **overwrite your global Conan configuration**.

```bash
conan config install ./conan/conan-config/config
```

---

### 💻 2. Desktop Build

```bash
# Create default profile
conan profile detect
```

```bash
# Install dependencies
# For Windows
conan install . -pr clang-cl-llvm -r skylabs -r conancenter -s build_type=Debug -s compiler.runtime_type=Debug --build=missing

# For Linux
conan install . -pr clang-linux -r skylabs -r conancenter -s build_type=Debug --build=missing

# Configure project
cmake --preset conan-debug

# Build
cmake --build build
```

## 📚 References & Resources
* [UTF-8 Everywhere](https://utf8everywhere.org/)
* [Canonical Project Structure](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html)
* [About console on Win32](https://www.devever.net/~hl/win32con)
* [Predefined macros](https://github.com/cpredef/predef)
* [Calendar Versioning](https://calver.org/)
* [Semantic Versioning](https://semver.org)
