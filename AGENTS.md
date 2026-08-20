# Uptooda development instructions

## Project overview

- Uptooda is a C++17 application for uploading images, screenshots, and other files to multiple file-hosting services.
- It can capture screenshots, record screen video, and edit images using its integrated image editor.
- The UI stack consists of Qt 6 and a legacy WTL UI.
- All C++ source code is located under `Source/`.
- Dependencies are managed with Conan 2. The main recipe is `Source/conanfile.py`.
- The project is configured and built with CMake.

## Formatting

- Format all new or modified C/C++ code according to `Source/.clang-format`.
- When practical, run `clang-format` on the touched C/C++ lines or files before considering a change complete.
- Avoid unrelated formatting changes in legacy code.

## Naming conventions

Apply these conventions to new code. Do not rename existing legacy symbols solely to make them conform.

| Symbol kind | Convention                                       |
| --- |--------------------------------------------------|
| Classes and structs | `UpperCamelCase`                                 |
| Concepts | `all_lower`                                      |
| Enums | `UpperCamelCase_UnderscoreTolerant`              |
| Unions | `UpperCamelCase_UnderscoreTolerant`              |
| Template parameters | `UpperCamelCase`                                 |
| Parameters | `lowerCamelCase`                                 |
| Local variables | `lowerCamelCase`                                 |
| Global variables | `UpperCamelCase_UnderscoreTolerant`              |
| Lambdas | `lowerCamelCase`                                 |
| Global functions | `UpperCamelCase_UnderscoreTolerant`              |
| Class and struct methods | `lowerCamelCase`                                 |
| Class and struct fields | `lowerCamelCase_`   (should end with underscore) |
| Class and struct public fields | `UpperCamelCase_UnderscoreTolerant`              |
| Union members | `UpperCamelCase_UnderscoreTolerant`              |
| Enumerators | `ALL_UPPER`                                      |
| Other constants | `ALL_UPPER`                                      |
| Global constants | `ALL_UPPER`                                      |
| Namespaces | `UpperCamelCase_UnderscoreTolerant`              |
| Typedefs | `UpperCamelCase_UnderscoreTolerant`              |
| Macros | `ALL_UPPER`                                      |
| Properties | `UpperCamelCase`                                 |
| Events | `UpperCamelCase`                                 |

`UnderscoreTolerant` means that underscores are permitted where needed while the named segments otherwise follow the stated casing style.

## CMake configuration and build

- Use `Build-VS2026-CLion-Static-Debug` as the build directory for the documented Debug configuration.
- Configure from the repository root with:

```powershell
cmake -S Source -B Build-VS2026-CLion-Static-Debug `
  -G "Visual Studio 18 2026" `
  "-DCMAKE_BUILD_TYPE=Debug" `
  "-DCMAKE_CONFIGURATION_TYPES=Debug" `
  "-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake" `
  "-DCONAN_HOST_PROFILE=d:/Develop/uptooda/Conan/Profiles/windows_vs2026_x64_debug" `
  "-DCMAKE_PREFIX_PATH=E:/Qt6/6.11.0/msvc2026_64_static" `
  "-DIU_BUILD_QIMAGEUPLOADER=On" `
  "-DIU_BUILD_WTLIMAGEUPLOADER=On" `
  "-DIU_ENABLE_ASAN=On" `
  "-DCONAN_BUILD_PROFILE=default"
```

- Keep each complete `-D<name>=<value>` argument quoted when invoking CMake from PowerShell. In particular, an unquoted `-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake` may be split incorrectly, preventing the Conan provider from loading and causing misleading missing-package errors such as a missing `Boost::program_options`.
- CMake configure must be able to read the user's Conan 2 configuration and package cache under `%USERPROFILE%\.conan2`. A denied-access error for that directory is an execution-environment restriction rather than a project configuration error.
- Conan warnings about deprecated Conan 1.x recipe properties are currently non-fatal. Qt messages about missing optional wrapper packages such as `WrapOpenXR`, `ODBC`, `WrapPNG`, and similar packages are also non-fatal when Qt reports that it is using a bundled or alternative implementation and configure finishes successfully.
- Treat configure as successful only when it ends with both `Configuring done` and `Generating done` and reports that build files were written to the expected build directory.

- Build the Debug configuration with:

```powershell
cmake --build Build-VS2026-CLion-Static-Debug --config Debug
```
