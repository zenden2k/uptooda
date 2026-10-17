from conan import ConanFile
from conan.tools.files import save, load, patch, copy
from conan.tools.cmake import cmake_layout, CMake, CMakeToolchain
from conan.tools.scm import Git

import shutil, os

class BaseClassesConan(ConanFile):
    name = "base-classes"
    version = "1.0.0"
    license = "Apache-2.0"
    url = "https://docs.microsoft.com/en-us/windows/win32/directshow/using-the-directshow-base-classes"
    description = "DirectShow Base Classes"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    generators = "CMakeToolchain", "CMakeDeps"
    exports_sources = "CMakeLists.txt", "*.cpp", "*.h" 
    
    def source(self):
        git = Git(self)
        git.clone(url="https://github.com/zenden2k/base-classes.git")
    
    def build(self):
        cmake = CMake(self)
        cmake.configure(build_script_folder="./base-classes")
        cmake.build()

    def package(self):
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        copy(self, pattern="*.h", src=os.path.join(self.source_folder, "base-classes"), dst=os.path.join(self.package_folder, "include"))
        copy(self, pattern="*.a", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, pattern="*.so", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, pattern="*.lib", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, pattern="*.dll", src=self.build_folder, dst=os.path.join(self.package_folder, "bin"), keep_path=False)
        copy(self, pattern="*.dylib", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["base-classes"]
        self.cpp_info.system_libs = ["strmiids"]