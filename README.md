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
| **C++ Compiler** | Modern MSVC / GCC / Clang |
| **Python**       | 3.x                       |
| **Conan**        | 2.x                       |
| **Vulkan SDK**   | Latest SDK                |

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

### 🌍 Optional: Install Global Conan Config

> [!CAUTION]
> This will **overwrite your global Conan configuration**.
> Use it only if you want a fully reproducible environment identical to this project.

```bash
conan config install ./conan/conan-config/config
```

---

### 💻 2. Desktop Build

#### 

```bash
# Install dependencies
conan install . -r skylabs -r conancenter --build=missing

# Configure project
cmake --preset conan-default

# Build
cmake --build build
```

---

### 📱 3. Android Build

The Android pipeline is **fully automated via Gradle** - no manual Conan step required.

```bash
cd android
./gradlew assembleDebug
```

> [!TIP]
> Gradle triggers Conan internally during the `preBuild` phase. It uses android-specific Conan profile (you can copy from my Conan config repo).

## 📚 References & Resources
* [UTF-8 Everywhere](https://utf8everywhere.org/)
* [Canonical Project Structure](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html)
* [About console on Win32](https://www.devever.net/~hl/win32con)
