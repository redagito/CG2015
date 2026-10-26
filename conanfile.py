from conan import ConanFile
from conan.tools.cmake import cmake_layout


class CG2015(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"
    requires = (
        "glfw/3.4", "glad/2.0.8", "glm/1.0.3", "jsoncpp/1.9.8",
        "tinyobjloader/2.0.0-rc10", "lodepng/cci.20260102",
        "spdlog/1.17.0",
        "miniaudio/0.11.25",
    )
    default_options = {
        "*:shared": False,
        "spdlog/*:use_std_fmt": True,
        "glad/*:gl_profile": "core",
        "glad/*:gl_version": "4.6",
        # A single NONE is false in this Conan recipe and accidentally selects ALL.
        # Passing a list keeps it truthy; glad's CMake recognizes the NONE sentinel.
        "glad/*:extensions": "NONE,NONE",
        "glad/*:debug_layer": False,
        "glad/*:no_loader": True,
    }

    def layout(self):
        cmake_layout(self)
