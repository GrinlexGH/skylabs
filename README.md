# 🥼 Skylabs

[![Discord](https://img.shields.io/badge/Discord-5865F2?logo=discord&logoColor=white)](https://discord.gg/YqTKmA5qbf)
[![C++](https://img.shields.io/badge/C++-23-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![Vulkan](https://img.shields.io/badge/Vulkan-SDK-red.svg?style=flat&logo=vulkan)](https://vulkan.lunarg.com/)

> [!NOTE]
> **About this project**
> This is my personal sandbox for testing modern C++ features, exploring project architectures, and experimenting with graphics programming.
>
> 🎯 **Abstract Goal:** To build a functional 3D game engine from scratch using C++.

## 📥 Cloning the Repository

This project uses **Git Submodules**. It is crucial to include them during the clone process.

### ✅ Correct way to clone
Use the `--recursive` flag to ensure all dependencies are downloaded:

```bash
git clone --recursive https://github.com/GrinlexGH/Skylabs.git
````

### ⚠️ If you already cloned (without recursive)

If the folder is already on your disk but the `dependencies` or `submodules` folders are empty, run this command in the project root:

```bash
git submodule update --init --recursive
```

## 🛠️ Build Requirements

Ensure you have the following tools installed before compiling:

| Tool | Version Requirement |
| :--- | :--- |
| **CMake** | Latest stable |
| **Compiler** | `MSVC`, `g++` (\>=13), or `clang++` (\>=15) |
| **Python** | \>= 3.8 |
| **Conan** | \>= 2.0.0 |
| **Vulkan SDK** | [Download Here](https://vulkan.lunarg.com/sdk/home) |

## ⚙️ Configuration & Installation

### 1\. Configure Conan Remotes

This project relies on a custom Conan recipe repository.

**Add the local recipe index:**

```bash
conan remote add skylabs ./conan/conan-recipes -t local-recipes-index -f
```

**Optional: Install Global Config**

> [\!CAUTION]
> **Read before running:** The command below will overwrite your global Conan configuration. Only use this if you want to sync with my specific environment settings.

```bash
conan config install https://github.com/GrinlexGH/conan-config.git
```

### 2\. Install Dependencies & Compile

If you encounter any compilation issues, please open an [Issue](https://github.com/GrinlexGH/Skylabs/issues).

```bash
# Install dependencies
conan install . -r skylabs -r conancenter --build=missing

# Build the project
cmake --preset conan-default
```

> [\!TIP]
> 📂 **Output Location:**
> All compiled binaries will be placed in the `.output` folder.

## 📚 References & Resources

Useful links regarding the tech stack and concepts used in this project:

  * [UTF-8 Everywhere](https://utf8everywhere.org/)
  * [Canonical Project Structure](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html)
  * [About console on Win32](https://www.devever.net/~hl/win32con)
