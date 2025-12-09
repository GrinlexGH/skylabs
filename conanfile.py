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
        "boost/*:shared": False,

        "glm/*:shared": False,  # shared glm does not work on windows
    }

    def requirements(self):
        self.requires("fmt/12.1.0")
        self.requires("simple_term_colors/0.1")
        self.requires("tinyobjloader/2.0.0rc13")
        self.requires("glm/1.0.2")
        self.requires("vulkan-headers/1.4.335")
        self.requires("vulkan-memory-allocator-hpp/3.3.0-rc")
        self.requires("benchmark/1.9.4")
        self.requires("sdl_image/3.2.4")
        self.requires("sdl/3.2.28")
        self.requires("boost/1.89.0")
        self.requires("steamworks_sdk/1.62")

    def layout(self):
        cmake_layout(self)
