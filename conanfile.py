import os
import shutil
from pathlib import Path

from conan import ConanFile
from conan.tools.cmake import CMakeConfigDeps, CMakeToolchain, cmake_layout

class SkylabsRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    package_type = "application"
    default_options = {
        "boost/*:with_nowide": True,
        "boost/*:with_container": True,
        "boost/*:with_container_hash": True,
        "boost/*:with_range": True,
        "boost/*:with_unordered": True,
    }

    def build_requirements(self):
        self.tool_requires("slang/2026.17")

    def requirements(self):
        self.requires("boost/1.92.0")
        self.requires("entt/4.0.0")
        self.requires("fmt/12.2.0")
        self.requires("frozen/cci.20260421")
        self.requires("glm/1.0.3")
        self.requires("sdl/3.4.16")
        self.requires("sdl_image/3.4.6")
        self.requires("sdl_mixer/3.2.4")
        self.requires("sdl_ttf/3.2.2")
        self.requires("tinyobjloader/2.0.0rc13")
        self.requires("vk-bootstrap/1.4.361")
        self.requires("vulkan-headers/1.4.362")
        self.requires("vulkan-memory-allocator/3.4.0")
        self.requires("vulkan-memory-allocator-hpp/3.4.0+2")

        if self.settings.os == "Android":
            self.requires("vulkan-validation-layers-android/1.4.357.0")

    def generate(self):
        deps = CMakeConfigDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        if self.settings.os == "Android":
            self.create_vulkan_validation_symlink()
        tc.generate()

    def layout(self):
        cmake_layout(self)

    def create_vulkan_validation_symlink(self):
        arch_map = {
            "armv8": "arm64-v8a",
            "armv7": "armeabi-v7a",
            "x86": "x86",
            "x86_64": "x86_64",
        }
        abi = arch_map.get(str(self.settings.arch))
        if not abi:
            return

        vvl_pkg = Path(self.dependencies["vulkan-validation-layers-android"].package_path)

        src = vvl_pkg / abi / "libVkLayer_khronos_validation.so"
        dst = Path(self.source_folder) / "android" / "app" / "src" / "main" / "jniLibs" / abi / "libVkLayer_khronos_validation.so"

        self._copy(src, dst)

    def _copy(self, src: Path, dst: Path):
        if dst.exists() or dst.is_symlink():
            if dst.is_dir() and not dst.is_symlink():
                shutil.rmtree(dst)
            else:
                dst.unlink()

        dst.parent.mkdir(parents=True, exist_ok=True)

        shutil.copy2(src, dst)

        self.output.info(f"Copied: {src} -> {dst}")
