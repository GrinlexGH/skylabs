from conan import ConanFile
from conan.tools.cmake import cmake_layout

class SkylabsRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    package_type = "application"

    default_options = {
        "sdl/*:shared": True,
        "sdl_image/*:shared": True,

        "sdl_image/*:with_libtiff": False,
        "sdl_image/*:with_libwebp": False,
        "sdl_image/*:with_avif": False,

        "boost/*:with_nowide": True,
    }

    def requirements(self):
        self.requires("fmt/12.1.0")
        self.requires("simple_term_colors/0.1")
        self.requires("tinyobjloader/2.0.0rc13")
        self.requires("glm/1.0.3")
        self.requires("benchmark/1.9.4")
        self.requires("vulkan-headers/1.4.338")
        self.requires("vulkan-memory-allocator-hpp/3.3.0")
        self.requires("sdl_image/3.2.6")
        self.requires("sdl/3.4.0")
        self.requires("boost/1.90.0")
        self.requires("steamworks_sdk/1.63")
        self.requires("frozen/master-20250729")

    def layout(self):
        cmake_layout(self)
