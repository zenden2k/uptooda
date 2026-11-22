# Building Uptooda on Windows

You will need these tools to build Uptooda from source files:
- Git 		https://git-scm.com/downloads
- Microsoft Visual Studio 2019 or newer (with C++ compiler)   https://visualstudio.microsoft.com/downloads/
- Windows 10 SDK
- ATL library for your platform (a part of Visual Studio)
- CMake (tested with v3.24.0)    https://cmake.org/
- Python 3    https://www.python.org/downloads/
- Conan v2.x (C++ package manager; tested with v2.11.0)   https://conan.io/

Ensure that your %PATH% environment 
variable includes the directory of your Python and CMake distribution. 
I recommend to install Conan using `pip` utility. 
You can install specific version of conan: `pip3 install conan==2.11.0`. 
Run this command to add conan remote:

```bash
conan remote add zenden2k https://conan.svistunov.dev --index 0
```
To generate a Visual Studio solution from CMakeLists.txt you need to create a directory for build files and call `cmake`:

```bash
mkdir Build
cd Build
```

The recommended way is to pass conan profile to the `cmake` command.

```bash
cmake <path_to_uptooda_source>\Source -G "Visual Studio 17 2022" -A Win32 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake -DCONAN_HOST_PROFILE=default -DCONAN_BUILD_PROFILE=default 
```

If you are cross-compiling (from Windows x64 to x86) and want to build FFmpeg libraries from Conan repository then you should use two profiles (`-DCONAN_HOST_PROFILE=windows_vs2019_x86_release -DCONAN_BUILD_PROFILE=windows_vs2019_x64_release`)

After that, you can open the generated project in Visual Studio and build it there. Also you can run the command:
```
cmake --build . --config Release
```

The same you can achieve using `cmake-gui` utility.
In cmake-gui you can see all compilation options and easily change them.

If you want to build Uptooda with separate FFmpeg libraries (`IU_ENABLE_FFMPEG=On` and `IU_FFMPEG_STANDALONE=On`), after building put them (avcodec.lib, avformat.lib, avutil.lib, swscale.lib) into `Contrib\Lib` directory and header files into `Contrib\Include` directory.

Some examples, nevermind:

```bash
cmake ..\Source -G "Visual Studio 16 2019" -A Win32 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake -DCONAN_HOST_PROFILE=vs2019_x86_debug -DCONAN_BUILD_PROFILE=vs2022_x64 -DIU_ENABLE_FFMPEG=On -DIU_ENABLE_WEBVIEW2=On 

cmake ..\Source -G "Visual Studio 16 2019" -A ARM64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake -DCONAN_HOST_PROFILE=../Conan/profiles/vs2019_arm64_release -DCONAN_BUILD_PROFILE=../Conan/profiles/vs2022_x64 -DIU_ENABLE_FFMPEG=On -DIU_ENABLE_WEBVIEW2=On -DIU_FFMPEG_STANDALONE=On -DIU_ENABLE_MEDIAINFO=Off -DCMAKE_CONFIGURATION_TYPES:STRING=Release 
```

For ARM64:
```bash
cmake ..\Source -G "Visual Studio 16 2019" -A ARM64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake -DCONAN_HOST_PROFILE=../Conan/profiles/vs2019_arm64_release -DCONAN_BUILD_PROFILE=../Conan/Profiles/vs2022_x64 -DIU_ENABLE_FFMPEG=On -DIU_ENABLE_WEBVIEW2=On -DIU_FFMPEG_STANDALONE=On -DIU_ENABLE_MEDIAINFO=Off
```

## Running Tests in Visual Studio IDE

Open *Tools* -> *Options* -> *Test Adapter for Google Test* -> *General*

Set *Additional test execution parameters*:

```
--gtest_filter=-NetworkClientTest.*
```

Save settings, then open *Test* -> *Test explorer* and run all the tests.

# Building on Ubuntu (CLI and Qt GUI)

Use the same approach with Conan to build uptooda-cli on Ubuntu 20.04.

Before compiling Uptooda install the following packages:

```
sudo apt-get install g++ git ninja-build python3-pip # cmake
pip3 install conan
conan remote add zenden2k https://conan.svistunov.dev --index 0
```

Note that Ubuntu 20.04 has CMake v3.16 but the project requires >= v3.17.
You can install the latest binary distribution from the official website https://cmake.org/

Configure and build the project:

```bash
git clone https://github.com/zenden2k/image-uploader.git
cd image-uploader
mkdir Build
cd Build
cmake ../Source  -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake
cmake --build .
```

If you want to compile uptooda (Qt GUI) you have to install Qt 5 dev libraries before compiling:
```bash
sudo apt install qt5-default
```

Pass `-DIU_BUILD_QIMAGEUPLOADER=On` option to the `cmake` command.

You may also need these packages for creating a .deb package:
```bash
sudo apt install autoconf automake libtool fakeroot
```

# Build packages for all platforms at once

To build the release I use a combination of Windows and WSL 2 (Ubuntu 20.04).

Run on Windows:

```bash
git config --global core.autocrlf false
conan remote add zenden2k https://conan.svistunov.dev --index 0
```

Allow creating symlinks (you can do it by turning on Developer Mode on Windows 11).

## Prerequisites  (on Windows)

 * msbuild that is a part of Microsoft Visual Studio 2019 or newer (with x86/x64/ARM64 C++ compiler)
 * Windows 10 SDK
 * ATL library for x86/x64/ARM64 platform (a part of Visual Studio)
 * python 3
 * git
 * cmake
 * conan (version >= 2.0)
 * nuget?
 * 7-Zip (7z.exe)
 * Inno Setup 6 (iscc.exe)
 * WSL 2 & Ubuntu 20.04+

Add Conan remote and install python libraries:

```bash
conan remote add zenden2k https://conan.svistunov.dev --index 0
pip install GitPython python-dotenv
```

## Prerequisites On WSL (Ubuntu Linux)

* git 
* conan (version >= 2.0)
* cmake (version >= 3.17)
* doxygen
* msgfmt (gettext)
* Qt 5 development libraries
* autoconf, automake, libtool, fakeroot for building Debian packages.

You can install most of them by running the command 
```bash
sudo apt-get install git g++ cmake ninja-build python3-pip doxygen gettext  autoconf automake libtool fakeroot qt5-default
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-aarch64-linux-gnu # for cross-compiling
pip3 install conan==2.11.0
```
Note that Ubuntu 20.04 has CMake v3.16 but the script requires >= v3.17.
You can install the latest binary distribution from the official website https://cmake.org/

All programs must be added to the PATH environment variable.
Copy `versioninfo.h` to the `Dist` directory. Edit this file and set the values ​​you need. The build number will be incremented automatically.
 
Run `python create_build.py` from Visual Studio Developer Command Prompt.

# 📒 Licenses of used libraries and code

## Libraries

- [WTL](https://sourceforge.net/projects/wtl/) under [Microsoft Public License](https://opensource.org/license/ms-pl-html)
- [libcurl](https://curl.se/) under [CURL license (MIT/X inspired)](https://curl.se/docs/copyright.html)
- [openssl](https://www.openssl.org) under [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)
- [zlib](zlib.net) under [zLib license](https://zlib.net/zlib_license.html)
- [Boost](https://www.boost.org) under [Boost Software License](https://www.boost.org/LICENSE_1_0.txt)
- [Qt](https://www.qt.io/) under [LGPL-3.0](https://www.gnu.org/licenses/lgpl-3.0.html#license-text)
- [pcre](https://www.pcre.org) under BSD license
- [pcre++](https://www.daemon.de/projects/pcrepp/) (modified) under [LGPLv2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
- [squirrel](http://squirrel-lang.org) (modified) under [MIT License](https://opensource.org/license/mit)
- [sqrat](https://scrat.sourceforge.net) (modified) under [zlib License](https://scrat.sourceforge.net/#license)
- [ffmpeg](https://www.ffmpeg.org) under LGPL-2.1-or-later license
- [tinyxml2](https://github.com/leethomason/tinyxml2) under [zLib license](https://zlib.net/zlib_license.html)
- [gumbo](https://github.com/google/gumbo-parser) under [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0) 
- [gumbo-query](https://github.com/lazytiger/gumbo-query) under [MIT License](https://opensource.org/license/mit)
- [minizip](https://www.winimage.com/zLibDll/minizip.html) under [zLib license](https://zlib.net/zlib_license.html)
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp) under [MIT License](https://opensource.org/license/mit)
- [glog](https://github.com/google/glog) under [BSD-3-Clause license](https://opensource.org/license/BSD-3-clause)
- [libwebp](https://github.com/webmproject/libwebp) under [BSD-3-Clause license](https://opensource.org/license/BSD-3-clause)
- [base64](https://github.com/aklomp/base64) under [BSD-2-Clause license](https://opensource.org/license/bsd-2-clause)
- [UTF8-CPP](https://github.com/nemtrif/utfcpp) under [Boost Software License 1.0](https://opensource.org/license/bsl-1-0)
- [libheif](https://github.com/strukturag/libheif) under [LGPL-3.0](https://www.gnu.org/licenses/lgpl-3.0.html#license-text)
- [WinToast](https://github.com/mohabouje/WinToast) under [MIT License](https://opensource.org/license/mit)
- [argparse](https://github.com/p-ranav/argparse) under [MIT License](https://opensource.org/license/mit)
- [xdgmime](https://gitlab.freedesktop.org/xdg/xdgmime) under LGPL License
- [dotenv-cpp](https://github.com/laserpants/dotenv-cpp) under [BSD-3-Clause license](https://opensource.org/license/BSD-3-clause)
- [Simple-Web-Server](https://github.com/eidheim/Simple-Web-Server) under [MIT License](https://opensource.org/license/mit)
- [ScreenCapture](https://github.com/WindowsNT/ScreenCapture) under [CPOL](https://www.codeproject.com/info/cpol10.aspx)
- [xxHash](xxHash) under [BSD 2-Clause License](https://opensource.org/license/BSD-2-Clause)

DirectShow BaseClasses (strmbasd.lib, strmbase.lib) - part of Windows SDK 7.1

**Libraries for building Mega.nz:**

- [Mega SDK](https://github.com/meganz/sdk) under [BSD-2-Clause license](https://opensource.org/license/bsd-2-clause)
- [Crypto++](https://www.cryptopp.com/) under [Boost Software License 1.0](https://opensource.org/license/bsl-1-0)
- [c-ares](https://c-ares.org/) under [MIT License](https://opensource.org/license/mit)
- [libuv](https://github.com/libuv/libuv) under [MIT License](https://opensource.org/license/mit)

**Libraries for building tests:**

- [Google Mock](https://github.com/google/googletest) under [BSD-3-Clause license](https://opensource.org/license/BSD-3-clause)

## Resources

Free icons by [Icons8](https://icons8.com)
