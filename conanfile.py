from conan import ConanFile
from conan.tools.cmake import cmake_layout
import configparser

class VideoMonitor(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        config = configparser.ConfigParser(allow_no_value=True)
        config.read("deps/video-viewer/conanfile.txt")
        
        for dep in config["requires"]:
            self.requires(dep)

    def layout(self):
        cmake_layout(self)