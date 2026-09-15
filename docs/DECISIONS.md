# Decisions
In this file, I'll be describing the issues and topics I encountered while writing this project, as well as the reasoning behind the current build system.

## Table of Contents
1. [Build Configurations](#build-configurations)
2. [Third-Party Libraries](#third-party-libraries)
   - [Git Submodules](#git-submodules)
   - [Pre-Building](#pre-building)
3. [Conan](#conan)
   - [Building From Scratch](#building-from-scratch)
   - [Missing `.pdb` Files](#missing-pdb-files)
   - [ConanCenter](#conancenter)
   - [Local Recipe Index](#local-recipe-index)
   - [Artifactory Server](#artifactory-server)


## Build Configurations

To ensure the codebase stays portable, I needed a reliable way to compile Skylabs with multiple
compilers and configurations.

Initially, I was using [VS Code CMake kits](https://gist.github.com/GrinlexGH/cffbe9727b7183d7044e2c4af378ffd2). CMake kits are great for quick prototyping: they automatically detect local compilers, map executable paths, and seamlessly integrate into VS Code. But they only work in VS Code, so they cannot be easily shared across different IDEs or CI/CD pipelines.

CMake 3.19 added **CMake Presets**:
* `CMakePresets.json` defines generic project presets (generators, build directories, flags).
* `CMakeUserPresets.json` (gitignored) allows overriding local toolchain and compiler paths.

But unfortunately JSON configuration files quickly become bloated, although I personally am not picky about syntax. This eventually led me to look for a cleaner solution, which I found in **Conan profiles**.

## Third-Party Libraries

This is perhaps one of the most painful topics for C/C++. I used several methods in developing Skylabs.

### Git Submodules

The simplest way to include dependencies is adding them as Git submodules and bringing them into the build system using `add_subdirectory()`.

But this approach has several significant drawbacks:
* **IDE Clutter:** Every third-party library injects its own targets and source trees into the Visual Studio solution.
* **Increased Build Time:** Dependencies are compiled alongside your main project every time.
* **Cache Invalidation:** Whenever the CMake cache is wiped or regenerated, all external dependencies have to be rebuilt from scratch.

### Pre-Building

To avoid constant recompilation, a cleaner strategy is compiling dependencies separately, installing them into system via `cmake --install`, and referencing them in the main project using `find_package()`.

To automate this pipeline, I built a custom tool called [Deps](https://github.com/GrinlexGH/deps). But soon I realized that I was reinventing **Conan**.

## Conan

Conan neatly solves both problems discussed above:
1. **Dependency Management:** It fetches or builds binaries once, caches them globally, and generates CMake target integration automatically.
2. **Clean Build Profiles:** Conan profiles replace bloated `CMakePresets.json` setup with clean, declarative TOML configuration files specifying compiler versions, standard libraries, architectures, and build types.

It turned out to be the perfect solution.

### Building From Scratch

As I studied Conan, I realized that **it's best to compile libraries from scratch**. The first four paragraphs of [this](https://docs.conan.io/2/examples/dev_flow/debug/step_into_dependencies.html) article explain why. These include an `_ITERATOR_DEBUG_LEVEL` mismatch error and ABI incompatibility between `MinGW` and `MSVC`.

### Missing `.pdb` Files

Another important debugging detail is handling MSVC `.pdb` files. By default, some libraries do not install their `.pdb` files into the package, causing MSVC to generate annoying warnings. To fix this, I set up a hook in the Conan config that automatically finds and copies the missing `.pdb` files into the final package folder — see [`BUILD.md`](./BUILD.md#missing-pdb-files) for the exact hook and how it's wired in.

### ConanCenter

I encountered several issues with the default **ConanCenter** recipes:
* **Slow updates:** New library versions often take a while to be merged.
* **Build system bugs:** For example, Boost's `b2` build system mistakenly recognizes MinGW's Clang as `clang-cl`, setting the wrong compilation options.
* **Outdated dependencies:** The SDL recipe required an old version of CMake that lacked a necessary generator for my setup.
* **Missing recipes:** Critical libraries for my stack, such as the Slang shader compiler, Steamworks SDK, and Vulkan validation layers for Android, were completely missing.

[Turned out](https://github.com/conan-io/conan/issues/20155#issuecomment-4913977776) that recipes in ConanCenter are contributed with common defaults and can't cover all possible variabilities.

So I found the solution in a **local recipe index**.

### Local Recipe Index

I decided to host my own [local recipe index](https://docs.conan.io/2/devops/devops_local_recipes_index.html). It contains [recipes](../conan/conan-recipes/recipes) that have been rewritten and updated immediately when a new version of the library is released.

I even wrote a custom Telegram bot which automatically monitors releases for these libraries. (The bot is currently sitting in a private repository as the code is still a bit too raw).

For the actual remote-registration command, see [`BUILD.md`](./BUILD.md#one-time-setup).

### Artifactory Server

To avoid recompiling on my laptop and PC every time, I set up my own [Artifactory server](https://docs.conan.io/2/tutorial/conan_repositories/setting_up_conan_remotes/artifactory/artifactory_ce_cpp.html). It's configured like a regular Docker server.

It works very poorly on NTFS, and the built-in Derby database sometimes prevents it from starting correctly when the server is suddenly shut down.

To fix all this, I use PostgreSQL as a separate Docker service, install the server on a BTRFS partition on my laptop with CachyOS.
