import os

from conan import ConanFile
from conan.tools.files import save, load, patch, copy
from conan.tools.cmake import cmake_layout, CMake
from conan.tools.scm import Git

import shutil

class GumboParserConan(ConanFile):
    name = "gumbo-parser"
    version = "0.10.1"
    license = "Apache-2.0"
    url = "https://github.com/google/gumbo-parser"
    description = "An HTML5 parsing library in pure C99"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    generators = "CMakeToolchain", "CMakeDeps"
    exports_sources = "CMakeLists.txt", "patches/*"

    def source(self):
        git = Git(self)
        git.clone(url="https://github.com/google/gumbo-parser.git",
                  target="gumbo-parser", args=["--branch", "v0.10.1", "--depth", "1"])
        patch(self, base_path="gumbo-parser",
              patch_file=os.path.join(self.export_sources_folder, "patches", "patch.diff"))

    def build(self):
        cmake = CMake(self)
        cmake.configure()        
        cmake.build()
    
    def layout(self):
        cmake_layout(self)

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["gumbo"] 