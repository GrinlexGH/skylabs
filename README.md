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

## 🪝 Pre-commit Hooks

> [!IMPORTANT]
> Using **pre-commit** is required before contributing.
> It trims trailing whitespace, fixes line endings, and prevents committing merge conflicts or invalid filenames.

### 📥 Installation & Setup

1. **Install pre-commit:**
    * **For CLI users (within Python venv):**
      ```bash
      uv pip install pre-commit
      ```
    * **For GitHub Desktop / GUI clients:** Install `pre-commit` **globally** (e.g., via `scoop install pre-commit` or system Python). GUI apps cannot access tools isolated inside a virtual environment.

2. **Register the git hooks:**
   ```bash
   pre-commit install

Hooks will run automatically before every git commit. To run checks manually on all files:

```bash
pre-commit run --all-files
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

See detailed guide in [BUILD.md](docs/BUILD.md)

### 🦐 0. Install requirements

> [!TIP]
> It is highly recommended to check out [Scoop](https://scoop.sh/), an incredibly convenient package manager for Windows.

On Windows, it is recommended to use [Visual Studio](https://visualstudio.microsoft.com/) with [LLVM/Clang](https://github.com/llvm/llvm-project/releases).

Latest version of Conan is available in pip:

```bash
uv venv

# For Windows
.venv\Scripts\activate
# For Linux
source .venv/bin/activate

uv pip install -U -r ./requirements.txt
```

### 🔧 1. Configure Conan

**Create default profile:**

```bash
conan profile detect --force
```

---

**Add the local remote:**

This project uses a custom Conan recipe index.

```bash
conan remote add skylabs ./conan/conan-recipes -t local-recipes-index -f
```

---

**Install global conan config:**

> [!CAUTION]
> This will **overwrite your global Conan configuration**.

```bash
conan config install ./conan/conan-config/config
```

---

### 🏗️ 2. Build

```bash
# Install dependencies
# For Windows
conan install . -pr clang-llvm -r skylabs -r conancenter -s build_type=Debug -s compiler.runtime_type=Debug --build=missing

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
