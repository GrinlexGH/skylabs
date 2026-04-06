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

        "sdl_mixer/*:with_gme": False,

        "boost/*:with_nowide": True,
        "boost/*:with_container_hash": True,
    }

    def requirements(self):
        self.requires("sdl/3.4.4")
        self.requires("sdl_image/3.4.2")
        self.requires("sdl_mixer/3.2.0")
        self.requires("boost/1.90.0")
        self.requires("steamworks_sdk/1.64")
        self.requires("vulkan-headers/1.4.347")
        self.requires("vulkan-memory-allocator-hpp/3.3.0+3")
        self.requires("vk-bootstrap/1.4.347")
        self.requires("spirv-reflect/system")
        self.requires("tinyobjloader/2.0.0rc13")
        self.requires("glm/1.0.3")
        self.requires("fmt/12.1.0")
        self.requires("simple_term_colors/0.1")
        self.requires("frozen/master-20250729")
        self.requires("catch2/3.14.0")

    def generate(self):
        sdl_pkg = self.dependencies["sdl"].package_folder
        sdl_java_src = os.path.abspath(os.path.join(sdl_pkg, "android-project", "app", "src", "main", "java", "org", "libsdl"))

        project_java_dir = os.path.join(self.recipe_folder, "android", "app", "src", "main", "java", "org", "libsdl")
        if os.path.exists(sdl_java_src):
            if not os.path.exists(project_java_dir):
                os.makedirs(os.path.dirname(project_java_dir), exist_ok=True)
                try:
                    os.symlink(sdl_java_src, project_java_dir, target_is_directory=True)
                    self.output.info(f"Symlink created: {project_java_dir} -> {sdl_java_src}")
                except Exception as e:
                    self.output.error(f"Failed to create symlink: {e}")

    def layout(self):
        cmake_layout(self)
