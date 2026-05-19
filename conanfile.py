import os

from conan import ConanFile
from conan.tools.cmake import cmake_layout

class SkylabsRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    package_type = "application"

    default_options = {
        "sdl_image/*:with_libtiff": False,
        "sdl_image/*:with_libwebp": False,
        "sdl_image/*:with_avif": False,

        "boost/*:with_nowide": True,
        "boost/*:with_container": True,
        "boost/*:with_container_hash": True,
        "boost/*:with_range": True,
        "boost/*:with_unordered": True,
    }

    def requirements(self):
        self.requires("sdl/3.4.8")
        self.requires("sdl_image/3.4.4")
        self.requires("sdl_mixer/3.2.2")
        self.requires("sdl_ttf/3.2.2")
        self.requires("boost/1.91.0-1")
        self.requires("steamworks_sdk/1.64")
        self.requires("vulkan-headers/1.4.352")
        self.requires("vulkan-memory-allocator-hpp/3.3.0+3")
        self.requires("vk-bootstrap/1.4.352")
        self.requires("entt/3.16.0")
        self.requires("tinyobjloader/2.0.0rc13")
        self.requires("glm/1.0.3")
        self.requires("fmt/12.1.0")
        self.requires("simple_term_colors/1.0")
        self.requires("frozen/cci.20260418")
        self.requires("stb/cci.20260313")
        self.requires("catch2/3.15.0")

        if self.settings.os == "Android":
            self.requires("vulkan-validation-layers-android/1.4.350.0")

    def generate(self):
        if self.settings.os == "Android":
            self.create_sdl_android_sources_symlink()
            self.create_vulkan_validation_symlink()

    def layout(self):
        cmake_layout(self)

    def create_sdl_android_sources_symlink(self):
        sdl_java_src = os.path.abspath(
            os.path.join(self.dependencies["sdl"].package_folder, "android-project", "app", "src", "main", "java", "org", "libsdl"
        ))
        project_java_dir = os.path.join(
            self.recipe_folder, "android", "app", "src", "main", "java", "org", "libsdl"
        )

        self._create_symlink(sdl_java_src, project_java_dir, target_is_directory=True)

    def create_vulkan_validation_symlink(self):
        arch_map = {
            "armv8": "arm64-v8a",
            "armv7": "armeabi-v7a",
            "x86": "x86",
            "x86_64": "x86_64",
        }
        android_abi = arch_map.get(str(self.settings.arch))
        if not android_abi:
            return

        vvl_bin = os.path.abspath(os.path.join(
            self.dependencies["vulkan-validation-layers-android"].package_folder, android_abi, "libVkLayer_khronos_validation.so"
        ))
        project_jniLibs_dir = os.path.join(
            self.source_folder, "android", "app", "src", "main", "jniLibs", android_abi, "libVkLayer_khronos_validation.so"
        )

        self._create_symlink(vvl_bin, project_jniLibs_dir)

    def _create_symlink(self, source, destonation, target_is_directory=False):
        if not os.path.exists(source):
            self.output.info(f"Path '{source}' doesn't exist")
            return

        if os.path.exists(destonation):
            os.remove(destonation)

        os.makedirs(os.path.dirname(destonation), exist_ok=True)
        try:
            os.symlink(source, destonation, target_is_directory=target_is_directory)
            self.output.info(f"Symlink created: {destonation} -> {source}")
        except Exception as e:
            self.output.warning(f"Failed to create symlink: {e}")
