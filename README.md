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
| **C++ Compiler** | Latest MSVC / GCC / Clang |
| **Conan**        | Latest version            |
| **Android SDK**  | Latest SDK (optional)     |

## ⚙️ Configuration & Building

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

> [!WARNING]
> This project is using C++ 20 modules, so you need to set `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` to uuid which your version of cmake uses. You can see it in cmake [sources](https://github.com/Kitware/CMake/blob/master/Help/dev/experimental.rst#c-import-std-support).

```bash
# Install dependencies
# Use msvc:
conan install . -pr msvc-18 -r skylabs -r conancenter -s build_type=Debug -s compiler.runtime_type=Debug --build=missing

# Use LLVM/clang from github:
conan install . -pr clang-cl-llvm -r skylabs -r conancenter -s build_type=Debug -s compiler.runtime_type=Debug --build=missing

# Use clang from visual studio:
conan install . -pr clang-cl-18 -r skylabs -r conancenter -s build_type=Debug -s compiler.runtime_type=Debug --build=missing

# Use mingw:
conan install . -pr clang-clang64 -r skylabs -r conancenter -s build_type=Debug --build=missing
conan install . -pr gcc-ucrt64 -r skylabs -r conancenter -s build_type=Debug --build=missing

# Linux:
conan install . -pr clang-linux -r skylabs -r conancenter -s build_type=Debug --build=missing
conan install . -pr gcc-linux -r skylabs -r conancenter -s build_type=Debug --build=missing

# Configure project
cmake --preset conan-default

# Build
cmake --build build
```

---

### 📱 3. Android Build

The Android pipeline is **fully automated via Gradle** - no manual Conan step required (you can change conan setup manually from [`build.gradle.kts`](android/app/build.gradle.kts) if you need).

```bash
cd android
./gradlew assembleDebug
```

> [!TIP]
> Gradle triggers conan internally during the `configureCMake` phase. It uses android-specific conan profile (you can copy it from my conan config repo).

## 📚 References & Resources
* [UTF-8 Everywhere](https://utf8everywhere.org/)
* [Canonical Project Structure](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html)
* [About console on Win32](https://www.devever.net/~hl/win32con)
* [Predefined macros](https://github.com/cpredef/predef)
* [Calendar Versioning](https://calver.org/)
* [Semantic Versioning](https://semver.org)
