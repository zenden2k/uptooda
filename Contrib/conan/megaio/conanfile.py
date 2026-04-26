from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.files import copy, get
from conan.tools.files import apply_conandata_patches, export_conandata_patches

import os

class MegaioConan(ConanFile):
    name = "megaio"
    version = "9.2.0"
    license = "BSD-2-Clause license"
    author = "mega.nz"
    url = "https://github.com/meganz/sdk"
    description = "Mega SDK"
    topics = ("mega.nz", "cloud")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False], 
        "fPIC": [True, False], 
        "with_openssl": [True, False],
        "with_mediainfo": [True, False],
        "with_freeimage": [True, False],
        "with_ffmpeg": [True, False],
        "with_libuv": [True, False],
        "with_qt": [True, False],
        "with_pdfium": [True, False],
        "with_readline" : [True, False],
        "enable_sync": [True, False],
        "enable_chat": [True, False]
    }
    default_options = {
        "shared": False, 
        "fPIC": True, 
        "with_openssl": True,
        "with_mediainfo": True,
        "with_freeimage": False,
        "enable_sync": True,
        "enable_chat": False,
        "with_ffmpeg": False,
        "with_libuv": False,
        "with_qt": False,
        "with_pdfium": False,
        "with_readline" : False
#        "libcurl/*:with_libssh2": True
    }
    generators = "CMakeDeps"
    requires = [
        "zlib/[>=1.2.11 <2]",
        "c-ares/[>=1.17.2]",
#        "gtest/[>=1.10.0]",
    ]

    def export_sources(self):
        export_conandata_patches(self)

    def source(self):
        get(self, **self.conan_data["sources"][self.version])
        apply_conandata_patches(self)
        
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["USE_OPENSSL"] = 1 if self.options.with_openssl else 0
        tc.cache_variables["USE_MEDIAINFO"] = 1 if self.options.with_mediainfo else 0
        tc.cache_variables["USE_FREEIMAGE"] = 1 if self.options.with_freeimage else 0
        if not self.options.with_freeimage:
            tc.cache_variables["ENABLE_ISOLATED_GFX"] = 0    
        tc.cache_variables["ENABLE_SYNC"] = 1 if self.options.enable_sync else 0
        tc.cache_variables["ENABLE_CHAT"] = 1 if self.options.enable_chat else 0
        tc.cache_variables["USE_FFMPEG"] = 1 if self.options.with_ffmpeg else 0
        tc.cache_variables["USE_LIBUV"] = 1 if self.options.with_libuv else 0
        tc.cache_variables["USE_PDFIUM"] = 1 if self.options.with_pdfium else 0    
        tc.cache_variables["USE_READLINE"] = 1 if self.options.with_readline else 0    
        tc.cache_variables["ENABLE_QT_BINDINGS"] = 1 if self.options.with_qt else 0
        tc.cache_variables["ENABLE_SDKLIB_EXAMPLES"] = 0
        tc.cache_variables["ENABLE_SDKLIB_TESTS"] = 0
        tc.cache_variables["ENABLE_SDKLIB_WERROR"] = 0
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(build_script_folder="sdk-9.2.0")
        cmake.build()
        
    def requirements(self):
        self.requires("sqlite3/[>=3.46.0]")
        self.requires("libsodium/1.0.21")
        if self.settings.os != "Windows":
            self.requires("icu/[>=73.2]")
        self.requires("libcurl/[>=8.6.0]")
        self.requires("cryptopp/[>=8.9.0]@zenden2k/stable")

        if self.options.with_openssl:
            self.requires("openssl/[>=1.1 <4]")

        if self.options.with_mediainfo:
            self.requires("libmediainfo/[>=22.03]")

        if self.options.with_freeimage:
            self.requires("freeimage/[>=3.18.0]")

        #if self.options.UseWebrtc:
        if self.options.with_libuv:
            self.requires("libuv/[>=1.44.2]")

        if self.options.with_ffmpeg:
           self.requires("ffmpeg/[>=5.1.2 <6]")

        if self.options.with_readline:
           self.requires("readline/[>=8.1]")

        if self.options.with_pdfium:
           self.requires("pdfium/[>=95.0.4629]")
       
    def package(self):
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        copy(self, pattern="*.h", src=os.path.join(self.source_folder, "sdk-9.2.0/include"), dst=os.path.join(self.package_folder, "include"))
        copy(self, pattern="*.a", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, pattern="*.so", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, pattern="*.lib", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, pattern="*.dll", src=self.build_folder, dst=os.path.join(self.package_folder, "bin"), keep_path=False)
        copy(self, pattern="*.dylib", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)

    def package_info(self):
        if self.settings.build_type == "Debug":
            self.cpp_info.libs.append("SDKlibd")
        else:
            self.cpp_info.libs.append("SDKlib")
   
        self.cpp_info.libs.append("ccronexpr")     

        if self.options.with_freeimage:
            self.cpp_info.libs.append("libgfxworker")
 
        if self.options.with_mediainfo:
            self.cpp_info.defines.append("USE_MEDIAINFO")
        
        if self.options.with_openssl:
            self.cpp_info.defines.append("USE_OPENSSL")
        
        if self.options.enable_sync:   
            self.cpp_info.defines.append("ENABLE_SYNC")
        
        if self.options.enable_chat:   
            self.cpp_info.defines.append("ENABLE_CHAT")
        
        if self.options.with_freeimage:
           self.cpp_info.defines.append("USE_FREEIMAGE")
    
        if self.options.with_ffmpeg:
           self.cpp_info.defines.append("HAVE_FFMPEG")  
           
        if self.options.with_libuv:
            self.cpp_info.defines.append("HAVE_LIBUV")   
            
        if self.options.with_pdfium:
            self.cpp_info.defines.append("HAVE_PDFIUM")   
                
                
        
        

