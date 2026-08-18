import sys
import re
import argparse
import datetime
import subprocess
import shutil
import os
import stat
import json
import hashlib
import xml.etree.ElementTree
import urllib.error
import urllib.request
import git

from contextlib import contextmanager
from dotenv import load_dotenv

def get_bool_env(env_var, default=False):
    value = os.getenv(env_var, str(default))
    return value.lower() in ('true', '1', 'yes', 'on')

load_dotenv()

IS_RELEASE = get_bool_env("UPTOODA_BUILD_RELEASE")
TEST_MODE = get_bool_env("UPTOODA_BUILD_TEST_MODE")
BUILD_DOCS = True
PDB_DIR = "PDB"
OUTDIR = "Releases" if IS_RELEASE else "Packages"
APP_NAME = "Uptooda"
IU_GIT_REPOSITORY = os.getenv("UPTOODA_BUILD_REPOSITORY", "https://github.com/zenden2k/uptooda.git")
GIT_BRANCH = os.getenv("UPTOODA_BUILD_BRANCH", "master")
GIT_COMMIT = os.getenv("UPTOODA_BUILD_COMMIT", "")
PARALLEL_JOBS = os.getenv("UPTOODA_BUILD_PARALLEL_JOBS", "6")
DOWNLOAD_CA_BUNDLE = True
DRDUMP_APP_GUID = "7b4202e6-8294-4be5-a18d-69c097167b46"
UPLOAD_TO_DRDUMP = get_bool_env("UPTOODA_BUILD_UPLOAD_TO_DRDUMP", True)
SYMUPLOAD_EXE = os.getenv("UPTOODA_BUILD_SYMUPLOAD_EXE", "SYMUPLOAD")
IS_WINDOWS_HOST = os.name == "nt"
LOCK_VERSION_HEADER = get_bool_env("UPTOODA_BUILD_LOCK_VERSION_HEADER", False)
GITHUB_REPOSITORY = os.getenv("UPTOODA_BUILD_GITHUB_REPOSITORY", "zenden2k/uptooda")
GITHUB_VARIABLES_TOKEN = os.getenv("GH_VARIABLES_TOKEN", os.getenv("GH_TOKEN", ""))

CMAKE_GENERATOR_VS2019 = "Visual Studio 16 2019"
CMAKE_GENERATOR_VS2022 = "Visual Studio 17 2022"
CMAKE_GENERATOR_VS2026 = "Visual Studio 17 2026"
WINDOWS_CMAKE_GENERATOR = os.getenv("UPTOODA_BUILD_WINDOWS_CMAKE_GENERATOR", CMAKE_GENERATOR_VS2026)

DEFAULT_BUILD_PROFILE = os.getenv("UPTOODA_BUILD_DEFAULT_BUILD_PROFILE", "default")
CONAN_PROFILES_PATH = os.getenv("UPTOODA_BUILD_CONAN_PROFILES_PATH", "").strip()
if CONAN_PROFILES_PATH:
    CONAN_PROFILES_PATH = os.path.expandvars(os.path.expanduser(CONAN_PROFILES_PATH))
SHELLEXT_PLATFORM_TOOLSET = os.getenv("UPTOODA_BUILD_SHELLEXT_PLATFORM_TOOLSET", "").strip()
WINDOWS_HOST_PROFILE_X86 = os.getenv("UPTOODA_BUILD_WINDOWS_HOST_PROFILE_X86", "windows_vs2019_x86_release")
WINDOWS_HOST_PROFILE_X64 = os.getenv("UPTOODA_BUILD_WINDOWS_HOST_PROFILE_X64", "windows_vs2019_x64_release")
WINDOWS_HOST_PROFILE_ARM64 = os.getenv("UPTOODA_BUILD_WINDOWS_HOST_PROFILE_ARM64", "windows_vs2019_arm64_release")

NUGET_ARCH_MAPPING = {
    'armv8': 'arm64',
    'x86_64': "x64",
    'x86': 'x86'
}

RESULT_ARCH_MAPPING = {
    'armv8': 'arm64',
    'x86_64': "x64",
    'x86': 'x86'
}

ALTERNATIVE_ARCH_DISPLAY_NAME = {
    'armv8': 'ARM64',
    'x86_64': "x86 64-bit",
    'x86': 'x86 32-bit',
    'aarch64': 'ARM64'
}

BUILD_TARGETS = [
    {
        'os': "Windows",
        'compiler': "VS2019",
        'build_type': "Release",
        'arch': 'x86_64',
        'host_profile': WINDOWS_HOST_PROFILE_X64,
        'build_profile': DEFAULT_BUILD_PROFILE,
        'cmake_generator': WINDOWS_CMAKE_GENERATOR,
        'cmake_platform': "x64",
        'cmake_args': ["-DIU_ENABLE_FFMPEG=On", "-DIU_BUILD_QIMAGEUPLOADER=On", "-DCMAKE_PREFIX_PATH=E:/Qt6/6.11.0/msvc2026_64_static_release"],
        'enable_webview2': True,
        'shell_ext_64bit_arch': 'x64',
        'ffmpeg_standalone' : False,
        'installer_arch': 'x64',
        'run_tests': True,
        'supported_os': 'Windows 10/11 (64 bit)',
        'upload_pdb': True
    },
    {
        'os': "Windows",
        'compiler': "VS2019",
        'build_type': "Release",
        'arch': 'armv8',
        'host_profile': WINDOWS_HOST_PROFILE_ARM64,
        'build_profile': DEFAULT_BUILD_PROFILE,
        'cmake_generator': WINDOWS_CMAKE_GENERATOR,
        'cmake_platform': "ARM64",
        'cmake_args': ["-DIU_ENABLE_FFMPEG=On", "-DIU_ENABLE_MEDIAINFO=Off", "-DIU_ENABLE_MEGANZ=Off", "-DIU_BUILD_QIMAGEUPLOADER=On"],
        'enable_webview2': True,
        'shell_ext_64bit_arch': 'ARM64',
        'ffmpeg_standalone' : False,
        'installer_arch': 'arm64',
        'supported_os': 'Windows 10/11 (ARM64)'
    },
    {
        'os': "Linux",
        'compiler': "gcc",
        'build_type': "Release",
        'arch': 'x86_64',
        'host_profile': 'default',
        'build_profile': DEFAULT_BUILD_PROFILE,
        'cmake_generator': 'Ninja Multi-Config',
        'cmake_args': ["-DIU_ENABLE_FFMPEG=On", "-DIU_BUILD_QIMAGEUPLOADER=On"],
        'deb_package_arch': 'amd64',
        'build_qt_gui': True,
        'run_tests': True,
        'supported_os': 'Linux (amd64)',
        'objcopy': 'objcopy'
    },
    {
        'os': "Linux",
        'compiler': "gcc",
        'build_type': "Release",
        'arch': 'aarch64',
        'host_profile': '',
        'build_profile': DEFAULT_BUILD_PROFILE,
        'cmake_generator': 'Ninja Multi-Config',
        'cmake_args': ["-DCMAKE_TOOLCHAIN_FILE=../Conan/Toolchains/aarch64-linux-gnu.toolchain.cmake"],
        'deb_package_arch': 'arm64',
        'build_qt_gui': False,
        'run_tests': False,
        'supported_os': 'Linux (arm64)',
        'objcopy': 'aarch64-linux-gnu-objcopy'
    },
]

COMMON_BUILD_FOLDER = "Build_Release_Temp"
VERSION_HEADER_FILE = "versioninfo-release.h" if IS_RELEASE else "versioninfo-nightly.h"
ENV_FILE = ".env"

# ---------------------------------------------------------------------------
# Dependency checking
# ---------------------------------------------------------------------------

class DepCheckResult:
    def __init__(self, name, ok, hint=''):
        self.name = name
        self.ok = ok
        self.hint = hint

def _run_check(args):
    try:
        r = subprocess.run(args, capture_output=True)
        return r.returncode == 0
    except Exception:
        return False

def _wsl_command(args):
    return [
        "wsl", "-e", "bash", "-lc",
        'export PATH="$HOME/.local/bin:$PATH"; '
        'if [ -n "${UPTOODA_BUILD_WSL_CONAN_HOME:-}" ]; then export CONAN_HOME="$UPTOODA_BUILD_WSL_CONAN_HOME"; fi; '
        'exec "$@"',
        "bash"
    ] + args

def _run_wsl_check(args):
    return _run_check(_wsl_command(args))

def _linux_command(args):
    return _wsl_command(args) if IS_WINDOWS_HOST else args

def _windows_conan_env():
    env = os.environ.copy()
    if IS_WINDOWS_HOST:
        # GitHub Actions/setup-python exports pkg-config paths that can leak
        # into Conan recipes executed under Windows bash, breaking ffmpeg.
        env.pop("PKG_CONFIG_PATH", None)
        env.pop("PKG_CONFIG_LIBDIR", None)
    return env

def _run_linux_check(args):
    return _run_check(_linux_command(args))

def _check_wsl_conan_version() -> bool:
    for args in (["conan", "--version"], ["python3", "-m", "conan", "--version"]):
        try:
            out = subprocess.check_output(_linux_command(args)).decode("utf-8").strip()
            m = re.match(r"Conan version (\d+)\.", out, re.IGNORECASE)
            if m:
                return int(m.group(1)) >= 2
        except Exception:
            pass
    return False

def check_dependencies(platform: str) -> bool:
    """
    Check all required dependencies for the given platform.
    platform: 'windows', 'linux', or 'all'
    Returns True if all required deps are satisfied.
    """
    print("\n=== Checking dependencies ===")

    common_deps = [
        DepCheckResult("git",   _run_check(["git", "--version"]),
                       "https://git-scm.com"),
        DepCheckResult("cmake", _run_check(["cmake", "--version"]),
                       "https://cmake.org"),
        DepCheckResult("python-dotenv", _check_python_package("dotenv"),
                       "pip install python-dotenv"),
        DepCheckResult("gitpython",     _check_python_package("git"),
                       "pip install gitpython"),
    ]

    windows_deps = [
        DepCheckResult("conan", _run_check(["conan", "--version"]),
                       "pip install conan"),
        DepCheckResult("msbuild",   _run_check(["msbuild", "--version"]),
                       "Запусти скрипт из Visual Studio Developer Command Prompt"),
        DepCheckResult("Inno Setup (iscc)", _check_innosetup(),
                       "https://jrsoftware.org/isinfo.php  —  добавь в PATH"),
        DepCheckResult("7zip (7z)",  _run_check(["7z", "--help"]),
                       "https://www.7-zip.org  —  добавь в PATH"),
    ]

    linux_deps = [
        DepCheckResult("WSL",              _run_check(["wsl", "--status"]),
                       "https://learn.microsoft.com/windows/wsl/install"),
        DepCheckResult("WSL cmake",        _run_wsl_check(["cmake", "--version"]),
                       "В WSL: sudo apt install cmake"),
        DepCheckResult("WSL git",          _run_wsl_check(["git", "--version"]),
                       "В WSL: sudo apt install git"),
        DepCheckResult("WSL conan",        _run_wsl_check(["conan", "--version"]) or _run_wsl_check(["python3", "-m", "conan", "--version"]),
                       "В WSL: запусти Dist/bootstrap.ps1"),
        DepCheckResult("WSL ninja",        _run_wsl_check(["ninja", "--version"]),
                       "В WSL: sudo apt install ninja-build"),
    ]

    if BUILD_DOCS:
        linux_deps.append(
            DepCheckResult("WSL doxygen", _run_wsl_check(["doxygen", "--version"]),
                           "В WSL: sudo apt install doxygen")
        )

    linux_prefix = "WSL " if IS_WINDOWS_HOST else ""
    linux_hint_prefix = "В WSL: " if IS_WINDOWS_HOST else ""
    linux_deps = []
    if IS_WINDOWS_HOST:
        linux_deps.append(
            DepCheckResult("WSL", _run_check(["wsl", "--status"]),
                           "https://learn.microsoft.com/windows/wsl/install")
        )
    linux_deps += [
        DepCheckResult(linux_prefix + "cmake", _run_linux_check(["cmake", "--version"]),
                       linux_hint_prefix + "sudo apt install cmake"),
        DepCheckResult(linux_prefix + "git", _run_linux_check(["git", "--version"]),
                       linux_hint_prefix + "sudo apt install git"),
        DepCheckResult(linux_prefix + "conan", _run_linux_check(["conan", "--version"]) or _run_linux_check(["python3", "-m", "conan", "--version"]),
                       linux_hint_prefix + "запусти Dist/bootstrap.ps1" if IS_WINDOWS_HOST else "pip install conan"),
        DepCheckResult(linux_prefix + "ninja", _run_linux_check(["ninja", "--version"]),
                       linux_hint_prefix + "sudo apt install ninja-build"),
    ]
    if BUILD_DOCS:
        linux_deps.append(
            DepCheckResult(linux_prefix + "doxygen", _run_linux_check(["doxygen", "--version"]),
                           linux_hint_prefix + "sudo apt install doxygen")
        )

    checks = list(common_deps)
    if platform in ("windows", "all"):
        checks += windows_deps
    if platform in ("linux", "all"):
        checks += linux_deps

    # Conan version check (must be 2.x)
    conan_ver_ok = True
    if platform in ("windows", "all"):
        conan_ver_ok = conan_ver_ok and _check_conan_version_silent()
    if platform in ("linux", "all"):
        conan_ver_ok = conan_ver_ok and _check_wsl_conan_version()

    all_ok = True
    for r in checks:
        status = "OK" if r.ok else "FAIL"
        print(f"  [{status}] {r.name}" + (f"  ->  {r.hint}" if not r.ok else ""))
        if not r.ok:
            all_ok = False

    if not conan_ver_ok:
        print("  [FAIL] conan version  ->  Conan 2.x required (pip install --upgrade conan)")
        all_ok = False

    if all_ok:
        print("  Все зависимости удовлетворены.\n")
    else:
        print("\n  Некоторые зависимости не найдены. Исправь проблемы выше и запусти снова.\n")

    return all_ok

def _check_python_package(module: str) -> bool:
    try:
        __import__(module)
        return True
    except ImportError:
        return False

def _check_innosetup() -> bool:
    try:
        out = subprocess.check_output(["iscc", "/?"], stderr=subprocess.STDOUT).decode("utf-8", errors="ignore")
        return "Inno Setup" in out
    except subprocess.CalledProcessError as e:
        return "Inno Setup" in (e.output or b"").decode("utf-8", errors="ignore")
    except Exception:
        return False

def _check_conan_version_silent() -> bool:
    try:
        out = subprocess.check_output(["conan", "--version"]).decode("utf-8").strip()
        m = re.match(r"Conan version (\d+)\.", out, re.IGNORECASE)
        if m:
            return int(m.group(1)) >= 2
        return False
    except Exception:
        return False

def normalize_shell_script_line_endings(root_dir):
    for current_dir, _, file_names in os.walk(root_dir):
        for file_name in file_names:
            if not file_name.endswith(".sh"):
                continue
            file_path = os.path.join(current_dir, file_name)
            with open(file_path, "rb") as file:
                content = file.read()
            normalized = content.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
            if normalized != content:
                with open(file_path, "wb") as file:
                    file.write(normalized)

def patch_shellext_platform_toolset(repo_dir):
    if not SHELLEXT_PLATFORM_TOOLSET:
        return

    project_file = os.path.join(repo_dir, "Source", "ShellExt", "ExplorerIntegration.vcxproj")
    if not os.path.isfile(project_file):
        print("Shell extension project not found:", project_file)
        return

    with open(project_file, "r", encoding="utf-8-sig") as file:
        text = file.read()
    patched = re.sub(
        r"<PlatformToolset>[^<]+</PlatformToolset>",
        f"<PlatformToolset>{SHELLEXT_PLATFORM_TOOLSET}</PlatformToolset>",
        text
    )
    if patched != text:
        with open(project_file, "w", encoding="utf-8", newline="") as file:
            file.write(patched)
        print("Shell extension PlatformToolset:", SHELLEXT_PLATFORM_TOOLSET)

# ---------------------------------------------------------------------------
# Original helpers (unchanged)
# ---------------------------------------------------------------------------

def check_program(args, message=''):
    try:
        proc = subprocess.run(args, capture_output=True)
        if proc.returncode == 0:
            return
    except Exception:
        pass
    print("Checking " + " ".join(args) + " failed")
    if message:
        print(message)
    sys.exit(1)

def get_recent_commits(repo_path, max_count=10, from_commit=None, to_commit=None):
    try:
        repo = git.Repo(repo_path)
        if from_commit and to_commit:
            commits = list(repo.iter_commits(f"{from_commit}..{to_commit}"))
        else:
            commits = list(repo.iter_commits(max_count=max_count))
        commit_list = []
        for commit in commits:
            commit_title = commit.message.strip().split('\n')[0]
            commit_info = {
                'commit_hash': commit.hexsha,
                'author': commit.author.name,
                'date': commit.committed_datetime.strftime('%a %b %d %H:%M:%S %Y %z'),
                'commit_message': commit_title
            }
            commit_list.append(commit_info)
        return commit_list
    except git.exc.InvalidGitRepositoryError:
        print(f"Error: {repo_path} is not a git repository")
        return []
    except Exception as e:
        print(f"Error retrieving commits: {e}")
        return []

def write_json_header(jsonfile, json_builds_file_name, source_dir, version_header_defines, git_commit_message):
    if os.path.exists(json_builds_file_name):
        with open(json_builds_file_name) as json_builds_file:
            json_builds_data = json.load(json_builds_file)
    else:
        json_builds_data = {}
    last_commit_hash = json_builds_data.get("last_commit_hash")
    now = datetime.datetime.now()
    dictionary = {
        "product": APP_NAME,
        "build_number":  version_header_defines['IU_BUILD_NUMBER'],
        "version":  version_header_defines['IU_APP_VER'],
        "version_clean": version_header_defines['IU_APP_VER_CLEAN'],
        "date": now.strftime('%Y-%m-%d %H:%M:%S'),
        "branch_name": version_header_defines['IU_BRANCH_NAME'],
        "commit_hash": version_header_defines['IU_COMMIT_HASH'],
        "commits": [],
        "files": []
    }
    arg = last_commit_hash
    if not last_commit_hash:
        arg = 'HEAD~1'
    commits = get_recent_commits(source_dir, 10, arg, "HEAD")
    dictionary['commits'] = commits
    with open(jsonfile, "w") as outfile:
        json.dump(dictionary, outfile, indent=4)
    with open(json_builds_file_name, "w") as outfile:
        json.dump(json_builds_data, outfile, indent=4)
    return dictionary

def write_builds_file(json_builds_file_name, version_header_defines):
    if os.path.exists(json_builds_file_name):
        with open(json_builds_file_name) as json_builds_file:
            json_builds_data = json.load(json_builds_file)
    else:
        json_builds_data = {}
    json_builds_data["last_commit_hash"] = version_header_defines['IU_COMMIT_HASH']
    with open(json_builds_file_name, "w") as outfile:
        json.dump(json_builds_data, outfile, indent=4)

def add_output_file(dictionary, target, jsonfile, name, path, relativePath, subproduct=''):
    if relativePath[0] == '/':
        relativePath = relativePath[1:]
    filename = os.path.basename(path)
    hash = calc_sha256_from_file(path)
    with open(path + ".sha256", "w") as hash_file:
        hash_file.write(hash + " *" + filename)
    arch_alt = str(ALTERNATIVE_ARCH_DISPLAY_NAME.get(target.get("arch")))
    file = {
        "name": name,
        "target_name": get_target_full_name(target),
        "arch": get_out_arch_name(target),
        "arch_alt": arch_alt,
        "compiler": target.get("compiler"),
        "os": target.get("os"),
        "filename": filename,
        "path": relativePath,
        "subproduct": subproduct,
        "sha256": hash,
        "size": os.path.getsize(path),
        "supported_os": target.get("supported_os"),
    }
    dictionary["files"] += [file]
    with open(jsonfile, "w") as outfile:
        json.dump(dictionary, outfile, indent=4)
    return dictionary

def calc_sha256_from_file(filepath):
    BUF_SIZE = 65536
    sha256 = hashlib.sha256()
    with open(filepath, 'rb') as f:
        while True:
            data = f.read(BUF_SIZE)
            if not data:
                break
            sha256.update(data)
    return sha256.hexdigest()

def mkdir_if_not_exists(dir):
    if not os.path.exists(dir):
        try:
            os.makedirs(dir)
        except OSError as error:
            print(error)
            exit(1)

def get_target_full_name(target):
    return "_".join([target["os"], target["compiler"], target["arch"], target["build_type"]])

def try_conan_host_profile(target, conan_profile_dir, profile_name):
    if not conan_profile_dir:
        if profile_name:
            return profile_name
        return get_target_full_name(target).lower()

    if profile_name:
        try_profile = conan_profile_dir + "/" + profile_name
    else:
        try_profile = conan_profile_dir + "/" + get_target_full_name(target).lower()
    if os.path.isfile(try_profile):
        if target["os"] == "Windows":
            return os.path.abspath(try_profile)
        else:
            return try_profile
    elif profile_name:
        return profile_name
    else:
        return "default"

def try_conan_build_profile(target, conan_profile_dir, profile_name, host_profile_name):
    if not conan_profile_dir:
        if profile_name:
            return profile_name
        if host_profile_name:
            return host_profile_name
        return "default"

    if profile_name:
        try_profile = conan_profile_dir + "/" + profile_name
        if os.path.isfile(try_profile):
            return os.path.abspath(try_profile)
        return profile_name
    elif host_profile_name:
        return host_profile_name
    return "default"

def del_rw(action, name, exc):
    os.chmod(name, stat.S_IWRITE)
    os.remove(name)

@contextmanager
def cwd(path):
    oldpwd = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(oldpwd)

def check_conan_version(args):
    conan_output = subprocess.check_output(args).decode("utf-8").strip()
    reg = re.compile(r"Conan version ([\d]+)\.[\d\.]+", flags=re.IGNORECASE)
    res = reg.match(conan_output)
    if res:
        conan_major_version = int(res.group(1))
        if conan_major_version < 2:
            print("Only Conan 2.x is supported.")
            sys.exit(1)
    else:
        print("Warning: Unknown Conan version")

def update_github_build_number():
    if not GITHUB_VARIABLES_TOKEN:
        raise RuntimeError("GH_VARIABLES_TOKEN is not set. Add it to Dist/.env or use --no-update-github-build-number.")

    variable_name = "IU_MODERN_BUILD_NUMBER"
    url = f"https://api.github.com/repos/{GITHUB_REPOSITORY}/actions/variables/{variable_name}"
    headers = {
        "Accept": "application/vnd.github+json",
        "Authorization": f"Bearer {GITHUB_VARIABLES_TOKEN}",
        "User-Agent": "uptooda-build-script",
        "X-GitHub-Api-Version": "2026-03-10",
    }
    try:
        with urllib.request.urlopen(urllib.request.Request(url, headers=headers)) as response:
            current_build_number = int(json.load(response)["value"])

        build_number = current_build_number + 1
        data = json.dumps({"name": variable_name, "value": str(build_number)}).encode("utf-8")
        request = urllib.request.Request(url, data=data, headers=headers, method="PATCH")
        with urllib.request.urlopen(request):
            pass
    except urllib.error.HTTPError as error:
        if error.code == 401:
            raise RuntimeError("GitHub rejected GH_VARIABLES_TOKEN: bad credentials. Create a new token and update Dist/.env.") from error
        raise RuntimeError(f"GitHub API request failed with HTTP {error.code}: {error.reason}") from error
    except (KeyError, TypeError, ValueError) as error:
        raise RuntimeError(f"GitHub variable {variable_name} does not contain a valid build number.") from error
    except urllib.error.URLError as error:
        raise RuntimeError(f"GitHub API request failed: {error.reason}") from error

    print(f"Updated GitHub variable {variable_name} to {build_number}.")
    return build_number

def generate_version_header(filename, inc_version, build_number_override=None):
    result = {}
    with open(filename) as f:
        content = f.readlines()
    content = [x.strip() for x in content]
    if LOCK_VERSION_HEADER:
        reg = re.compile("#define ([a-zA-Z0-9_]+) \"(.*?)\"")
        for line in content:
            res = reg.match(line)
            if res:
                result[res.group(1)] = str(res.group(2))
        return result
    reg = re.compile("#define ([a-zA-Z0-9_]+) \"(.*?)\"")
    out_text = ""
    for line in content:
        res = reg.match(line)
        if res:
            define_name = res.group(1)
            result[define_name] = str(res.group(2))
            if define_name == "IU_BUILD_NUMBER":
                if build_number_override is not None:
                    build_number = build_number_override
                    print("New IU build: {}".format(build_number))
                elif inc_version:
                    build_number = int(res.group(2)) + 1
                    print("New IU build: {}".format(build_number))
                else:
                    build_number = int(res.group(2))
                result[define_name] = str(build_number)
                out_text += "#define {} \"{}\"\n".format(define_name, str(build_number))
            elif define_name == "IU_APP_VER":
                if IS_RELEASE:
                    result[define_name] = result[define_name].replace("-nightly", "")
                else:
                    result[define_name] = now.strftime("%Y%m%d-nightly")
                out_text += "#define {} \"{}\"\n".format(define_name, str(result[define_name]))
            elif define_name == "IU_BUILD_DATE":
                now = datetime.datetime.now()
                out_text += "#define {} \"{}\"\n".format(define_name, now.strftime("%d.%m.%Y"))
            elif define_name == "IU_COMMIT_HASH":
                git_hash = subprocess.check_output(['git', 'rev-parse', 'HEAD']).decode("utf-8").strip()
                out_text += "#define {} \"{}\"\n".format(define_name, git_hash)
            elif define_name == "IU_COMMIT_HASH_SHORT":
                git_hash = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode("utf-8").strip()
                out_text += "#define {} \"{}\"\n".format(define_name, git_hash)
            elif define_name == "IU_BRANCH_NAME":
                git_branch_name = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).decode("utf-8").strip()
                out_text += "#define {} \"{}\"\n".format(define_name, git_branch_name)
            else:
                out_text += line + "\n"
        else:
            out_text += line + "\n"
    text_file = open(filename, "w")
    text_file.write(out_text)
    text_file.close()
    return result

def get_out_arch_name(target):
    src_arch = target.get("arch")
    if RESULT_ARCH_MAPPING.get(src_arch):
        return RESULT_ARCH_MAPPING.get(src_arch)
    return src_arch

def modify_update_file(component_name, filepath, version_header_defines, json_data):
    et = xml.etree.ElementTree.parse(filepath)
    root = et.getroot()
    now = datetime.datetime.now()
    root.attrib['TimeStamp'] = str(int(now.timestamp()))
    root.attrib['Date'] = now.strftime('%Y-%m-%d %H:%M:%S')
    root.attrib['DisplayName'] = component_name + " " + version_header_defines['IU_APP_VER'] + " build " + version_header_defines['IU_BUILD_NUMBER']
    root.attrib['DownloadPage'] = "https://svistunov.dev/uptooda_downloads" if IS_RELEASE else "https://svistunov.dev/uptooda_nightly"
    text = ''
    for commit in json_data['commits']:
        text += "- " + commit['commit_message'] + "\n"
    root.find('Info').text = text
    et.write(filepath, encoding='utf-8', xml_declaration=True)

# ---------------------------------------------------------------------------
# CLI argument parsing
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description=f"Build script for {APP_NAME}",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python build.py                        # build all platforms
  python build.py --platform windows     # Windows targets only
  python build.py --platform linux       # Linux targets only
  python build.py --check-deps           # check dependencies and exit
  python build.py --arch x86_64         # only x86_64 targets
  python build.py --platform windows --arch x86,armv8  # multiple arches
  python build.py --list-targets         # list available targets and exit
  python build.py --branch develop       # build from a specific branch
        """
    )

    parser.add_argument(
        "--platform", "-p",
        choices=["all", "windows", "linux"],
        default="all",
        help="Which OS targets to build (default: all)"
    )
    parser.add_argument(
        "--arch", "-a",
        default=None,
        help="Comma-separated list of architectures to build, e.g. x86_64,x86,armv8,aarch64"
    )
    parser.add_argument(
        "--check-deps",
        action="store_true",
        help="Check dependencies and exit without building"
    )
    parser.add_argument(
        "--skip-dep-check",
        action="store_true",
        help="Skip dependency check before building"
    )
    parser.add_argument(
        "--list-targets",
        action="store_true",
        help="List all available build targets and exit"
    )
    parser.add_argument(
        "--branch", "-b",
        default=None,
        help=f"Git branch to build (default: {GIT_BRANCH})"
    )
    parser.add_argument(
        "--no-tests",
        action="store_true",
        help="Skip running tests after build"
    )
    parser.add_argument(
        "--no-docs",
        action="store_true",
        help="Skip documentation generation"
    )
    parser.add_argument(
        "--jobs", "-j",
        default=None,
        help=f"Number of parallel build jobs (default: {PARALLEL_JOBS})"
    )
    parser.add_argument(
        "--update-github-build-number",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Read, increment and save the IU_BUILD_NUMBER GitHub variable (default: enabled)"
    )

    return parser.parse_args()


def filter_targets(targets, platform, arch_filter):
    """Return subset of BUILD_TARGETS matching platform and arch filters."""
    result = []
    arch_list = [a.strip() for a in arch_filter.split(",")] if arch_filter else None

    for t in targets:
        target_os = t["os"].lower()
        if platform != "all" and target_os != platform:
            continue
        if arch_list and t.get("arch") not in arch_list:
            continue
        result.append(t)
    return result


def list_targets(targets):
    print(f"\nAvailable build targets ({len(targets)} total):\n")
    print(f"  {'#':<3} {'OS':<10} {'Toolchain':<13} {'Arch':<10} {'Type':<10} {'Supported OS'}")
    print("  " + "-" * 72)
    for i, t in enumerate(targets):
        toolchain = "Visual Studio" if t["os"] == "Windows" else t["compiler"]
        print(f"  {i:<3} {t['os']:<10} {toolchain:<13} {t['arch']:<10} {t['build_type']:<10} {t.get('supported_os', '')}")
    print()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    global BUILD_DOCS, PARALLEL_JOBS

    args = parse_args()

    # Apply CLI overrides
    if args.branch:
        git_branch = args.branch
    else:
        git_branch = GIT_BRANCH

    if args.jobs:
        PARALLEL_JOBS = args.jobs

    if args.no_docs:
        BUILD_DOCS = False

    # --list-targets
    if args.list_targets:
        list_targets(BUILD_TARGETS)
        sys.exit(0)

    # Filter targets early so dep check knows what's needed
    active_targets = filter_targets(BUILD_TARGETS, args.platform, args.arch)
    if not active_targets:
        print(f"[ERROR] No targets match platform='{args.platform}' arch='{args.arch}'.")
        print("Use --list-targets to see available targets.")
        sys.exit(1)

    # Determine effective platform for dep check
    active_oses = {t["os"].lower() for t in active_targets}
    if active_oses == {"windows"}:
        dep_platform = "windows"
    elif active_oses == {"linux"}:
        dep_platform = "linux"
    else:
        dep_platform = "all"

    # Dependency check
    if args.check_deps:
        ok = check_dependencies(dep_platform)
        sys.exit(0 if ok else 1)

    if not args.skip_dep_check:
        ok = check_dependencies(dep_platform)
        if not ok:
            print("[ERROR] Dependency check failed. Fix the issues above or use --skip-dep-check to bypass.")
            sys.exit(1)

    print(f"\n=== Building {len(active_targets)} target(s) for platform '{args.platform}' ===\n")
    list_targets(active_targets)

    # -----------------------------------------------------------------------
    # Everything below is the original build logic, unchanged
    # -----------------------------------------------------------------------

    mkdir_if_not_exists(OUTDIR)
    mkdir_if_not_exists(PDB_DIR)
    outdir_abs = os.path.abspath(OUTDIR)
    pdbdir_abs = os.path.abspath(PDB_DIR)
    repo_dir = COMMON_BUILD_FOLDER + "/Repo"

    if not TEST_MODE:
        if os.path.exists(COMMON_BUILD_FOLDER):
            print("Directory exists, cleaning directory...")
            shutil.rmtree(COMMON_BUILD_FOLDER, onerror=del_rw)

    if not os.path.exists(COMMON_BUILD_FOLDER):
        try:
            os.mkdir(COMMON_BUILD_FOLDER)
        except OSError as error:
            print(error)
            exit(1)

    if os.path.exists(repo_dir) and not TEST_MODE:
        print("Directory exists, clearing directory...")
        shutil.rmtree(repo_dir, onerror=del_rw)

    if not os.path.exists(repo_dir):
        proc = subprocess.run(["git", "clone", "-b", git_branch, IU_GIT_REPOSITORY, repo_dir])
        if proc.returncode != 0:
            print("Git clone failed to directory " + repo_dir)
            exit(1)
    if GIT_COMMIT:
        proc = subprocess.run(["git", "checkout", GIT_COMMIT], cwd=repo_dir)
        if proc.returncode != 0:
            print("Git checkout failed for commit " + GIT_COMMIT)
            exit(1)

    if DOWNLOAD_CA_BUNDLE:
        proc = subprocess.run(["perl", "mk-ca-bundle.pl", "curl-ca-bundle.crt"])
        if proc.returncode != 0:
            print("Failed to download curl-ca-bundle")
            exit(1)

    curl_ca_bundle = os.path.abspath("curl-ca-bundle.crt")

    git_commit_message = subprocess.check_output(["git", "log", "-1", "--pretty=%B"]).decode("utf-8").strip()
    if not os.path.exists(VERSION_HEADER_FILE):
        shutil.copyfile("../Source/versioninfo.h.dist", VERSION_HEADER_FILE)

    version_file_abs_path = os.path.abspath(VERSION_HEADER_FILE)
    env_file_abs_path = os.path.abspath(ENV_FILE)

    github_build_number = None
    if args.update_github_build_number:
        try:
            github_build_number = update_github_build_number()
        except RuntimeError as error:
            print(f"[ERROR] {error}")
            sys.exit(1)

    generate_version_header(VERSION_HEADER_FILE, True, github_build_number)
    repo_dir_abs = os.path.abspath(repo_dir)
    normalize_shell_script_line_endings(repo_dir_abs)
    patch_shellext_platform_toolset(repo_dir_abs)
    shutil.copyfile(VERSION_HEADER_FILE, repo_dir + "/Source/versioninfo.h")
    shutil.copyfile(curl_ca_bundle, repo_dir + "/Dist/curl-ca-bundle.crt")
    shutil.copyfile("../Data/" + ENV_FILE, repo_dir + "/Data/" + ENV_FILE)

    dist_directory = os.path.dirname(os.path.realpath(__file__))
    version_header_defines = generate_version_header(repo_dir_abs + "/Source/versioninfo.h", False)
    app_ver = version_header_defines["IU_APP_VER"]
    build_number = version_header_defines["IU_BUILD_NUMBER"]

    proc = subprocess.run(_linux_command(["/bin/bash", "generate_mo.sh"]), cwd=repo_dir_abs + "/Lang/")
    if proc.returncode != 0:
        print("Cannot generate language files")

    if BUILD_DOCS:
        proc = subprocess.run(_linux_command(["/bin/bash", "generate.sh"]), cwd=repo_dir_abs + "/Dist/DocGen/")
        if proc.returncode != 0:
            print("Cannot generate documentation")

    new_build_dir = outdir_abs + "/" + app_ver + "-build-" + build_number
    mkdir_if_not_exists(new_build_dir)
    json_file_path = new_build_dir + "/build_info.json"
    json_builds_info_file = outdir_abs + "/builds.json"
    json_data = write_json_header(json_file_path, json_builds_info_file, repo_dir_abs, version_header_defines, git_commit_message)
    used_dist_dir = "/Dist/"

    src_xml_file = repo_dir_abs + "/Data/Update/iu_core.xml"
    modify_update_file(APP_NAME, src_xml_file, version_header_defines, json_data)
    shutil.copyfile(src_xml_file, new_build_dir + '/' + ("iu_core.xml" if IS_RELEASE else "iu_core_nightly.xml"))
    src_xml_file = repo_dir_abs + "/Data/Update/iu_serversinfo.xml"
    modify_update_file("Servers update", src_xml_file, version_header_defines, json_data)
    shutil.copyfile(src_xml_file, new_build_dir + '/' + ("iu_serversinfo.xml" if IS_RELEASE else "iu_serversinfo_nightly.xml"))

    for idx, target in enumerate(active_targets):
        target_full_name = get_target_full_name(target)
        build_dir_path = COMMON_BUILD_FOLDER + "/Build_" + target_full_name + "_" + str(idx)
        if not os.path.exists(build_dir_path):
            try:
                os.mkdir(build_dir_path)
            except OSError as error:
                print(error)
                exit(1)
        build_dir_path_abs = os.path.abspath(build_dir_path)
        host_profile = try_conan_host_profile(target, CONAN_PROFILES_PATH, target.get("host_profile"))
        build_profile = try_conan_build_profile(target, CONAN_PROFILES_PATH, target.get("build_profile"), host_profile)
        arch = target.get("arch")
        print("Building target:", target_full_name)
        print("Conan host profile:", host_profile)
        print("Conan build profile:", build_profile)

        with cwd(build_dir_path):
            include_path = "include"
            lib_path = "lib"
            mkdir_if_not_exists(include_path)
            mkdir_if_not_exists(lib_path)
            link_path = repo_dir_abs + "/Build"
            if os.path.islink(link_path):
                os.unlink(link_path)
            os.symlink(build_dir_path_abs, link_path)

            if target.get('ffmpeg_standalone'):
                src_dir = dist_directory + "/Libs/FFmpeg/" + target.get("arch") + "/lib/"
                if not os.path.exists(src_dir):
                    print("You must put ffmpeg libraries into", src_dir)
                files = os.listdir(src_dir)
                for fname in files:
                    shutil.copy2(os.path.join(src_dir, fname), lib_path)
                src_dir = dist_directory + "/Libs/FFmpeg/" + target.get("arch") + "/include/"
                shutil.copytree(src_dir, include_path, dirs_exist_ok=True)
                src_dir = dist_directory + "/Libs/FFmpeg/" + target.get("arch") + "/bin/"
                dest_dir = "GUI/Release/"
                shutil.copytree(src_dir, dest_dir, dirs_exist_ok=True, ignore=shutil.ignore_patterns('*.lib'))

            build_type = target.get("build_type")
            command = ["cmake", "../Repo/Source", "-G", target.get("cmake_generator"), "-DCMAKE_BUILD_TYPE=" + build_type,
                       "-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake",
                       "-DCONAN_BUILD_PROFILE=" + build_profile,
                       ]
            if target.get("host_profile") != "":
                command += ["-DCONAN_HOST_PROFILE=" + host_profile]
            if target.get("cmake_platform"):
                command += ["-A", target.get("cmake_platform")]
            if target.get("os") == "Linux":
                command = _linux_command(command)
            if target.get('ffmpeg_standalone'):
                command += ["-DIU_FFMPEG_STANDALONE=On"]
            if target.get("enable_webview2"):
                command += ["-DIU_ENABLE_WEBVIEW2=On"]
            if target.get("cmake_args"):
                command += target.get("cmake_args")

            print("Running command:", " ".join(command))
            proc_env = _windows_conan_env() if target.get("os") == "Windows" else None
            proc = subprocess.run(command, env=proc_env)
            if proc.returncode != 0:
                print("Generate failed")
                exit(1)

            command = ["cmake", "--build", ".", "-j", PARALLEL_JOBS, "--config", target.get("build_type")]
            if target.get("os") == "Linux":
                command = _linux_command(command)

            print("Running command:", " ".join(command))
            proc = subprocess.run(command)
            if proc.returncode != 0:
                print("Build failed")
                exit(1)

            run_tests = target.get("run_tests") and not args.no_tests
            if run_tests:
                command = ["Tests/Release/Tests"]
                if target.get("os") == "Linux":
                    command = _linux_command(command)
                print("Running command:", " ".join(command))
                proc = subprocess.run(command)
                if proc.returncode != 0:
                    print("Tests run failed")
                    exit(1)

            if target["os"] == "Windows":
                if target.get("shell_ext_arch"):
                    command = ["C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe", "..\\Repo\\Source\\ShellExt\\ExplorerIntegration.sln",
                               "/p:Configuration=ReleaseOptimized;Platform=" + target["shell_ext_arch"]]
                    print("Running command:", " ".join(command))
                    proc = subprocess.run(command)
                    if proc.returncode != 0:
                        print("Shell extension build failed")
                        exit(1)

                if target.get("shell_ext_64bit_arch"):
                    command = ["C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe", "..\\Repo\\Source\\ShellExt\\ExplorerIntegration.sln",
                               "/p:Configuration=ReleaseOptimized;Platform=" + target["shell_ext_64bit_arch"]]
                    print("Running command:", " ".join(command))
                    proc = subprocess.run(command)
                    if proc.returncode != 0:
                        print("Shell extension 64 bit Build failed")
                        exit(1)

                relative_path = r"/Windows/" + get_out_arch_name(target) + '/'
                package_os_dir = new_build_dir + relative_path
                mkdir_if_not_exists(package_os_dir)
                pdb_os_dir = pdbdir_abs + "/" + app_ver + "-build-" + build_number + relative_path
                mkdir_if_not_exists(pdb_os_dir)

                command = repo_dir_abs + used_dist_dir + r"create_portable.bat"
                print("Running command:", command)
                proc = subprocess.run(command, cwd=repo_dir_abs + used_dist_dir)
                if proc.returncode != 0:
                    print("Create archive failed")
                    exit(1)

                appname_suffix = " Lite" if target.get("lite") else ""
                output_filename_suffix = "-lite" if target.get("lite") else ""
                file_from = r"output\uptooda-" + app_ver + "-build-" + build_number + "-openssl-portable.7z"

                filename = "uptooda-" + app_ver + "-build-" + build_number + output_filename_suffix + "-" + get_out_arch_name(target) + ".7z"
                file_to = package_os_dir + "\\" + filename
                print("Copy file from:", file_from)
                print("Copy file to:", file_to)
                shutil.copyfile(file_from, file_to)
                json_data = add_output_file(json_data, target, json_file_path, "7zip archive", file_to, relative_path + filename, APP_NAME + " (GUI" + appname_suffix + ")")

                if not target.get("lite"):
                    command = repo_dir_abs + used_dist_dir + r"create_cli.bat"
                    print("Running command:", command)
                    proc = subprocess.run(command, cwd=repo_dir_abs + used_dist_dir)
                    if proc.returncode != 0:
                        print("Create archive failed")
                        exit(1)

                    file_from = r"output\\uptooda-cli-{version}-build-{build}.7z".format(version=app_ver, build=build_number)
                    filename = "uptooda-cli-{version}-build-{build}{suffix}-{arch}.7z".format(
                        version=app_ver, build=build_number, suffix=output_filename_suffix, arch=get_out_arch_name(target))
                    file_to = package_os_dir + "\\" + filename
                    print("Copy file from:", file_from)
                    print("Copy file to:", file_to)
                    shutil.copyfile(file_from, file_to)
                    json_data = add_output_file(json_data, target, json_file_path, "7zip archive", file_to, relative_path + filename, APP_NAME + " (CLI" + appname_suffix + ")")

                args_iscc = ["iscc.exe", "/dWIN64FILES", "/O" + build_dir_path_abs + r"\Installer"]
                if target.get('ffmpeg_standalone'):
                    args_iscc += ["/dIU_FFMPEG_STANDALONE"]
                args_iscc += ["/dIU_ARCH=" + str(target.get('installer_arch') or '')]
                args_iscc += [repo_dir_abs + used_dist_dir + "iu_setup_script.iss"]
                print("Running command:", " ".join(args_iscc))
                proc = subprocess.run(args_iscc, cwd=repo_dir_abs + used_dist_dir)
                if proc.returncode != 0:
                    print("Create installer failed")
                    exit(1)

                file_from = r"Installer\uptooda-" + app_ver + "-build-" + build_number + "-setup.exe"
                filename = "uptooda-" + app_ver + "-build-" + build_number + output_filename_suffix + "-" + get_out_arch_name(target) + "-setup.exe"
                file_to = package_os_dir + filename
                print("Copy file from:", file_from)
                print("Copy file to:", file_to)
                shutil.copyfile(file_from, file_to)
                json_data = add_output_file(json_data, target, json_file_path, "Installer", file_to, relative_path + filename, APP_NAME + " (GUI" + appname_suffix + ")")

                shutil.copyfile("GUI\\Release\\Uptooda.pdb", pdb_os_dir + "/Uptooda.pdb")

                if UPLOAD_TO_DRDUMP and target.get("upload_pdb"):
                    if not shutil.which(SYMUPLOAD_EXE):
                        print("SYMUPLOAD executable not found. Set UPTOODA_BUILD_SYMUPLOAD_EXE or add SYMUPLOAD.exe to PATH.")
                        exit(1)

                    command = [SYMUPLOAD_EXE, DRDUMP_APP_GUID, version_header_defines["IU_APP_VER_CLEAN"] + "." + build_number, "0", ".\\GUI\\Release\\*.pdb"]
                    proc = subprocess.run(command)
                    if proc.returncode != 0:
                        print("Failed to upload PDB files to DrDump server")
                        exit(1)

                    command = [SYMUPLOAD_EXE, DRDUMP_APP_GUID, version_header_defines["IU_APP_VER_CLEAN"] + "." + build_number, "0", ".\\GUI\\Release\\*.exe"]
                    proc = subprocess.run(command)
                    if proc.returncode != 0:
                        print("Failed to upload EXE files to DrDump server")
                        exit(1)

            elif target["os"] == "Linux":
                args_pkg = _linux_command(["/bin/bash", "create-package.sh", target.get("deb_package_arch"), target.get("objcopy")])
                working_dir = repo_dir_abs + used_dist_dir + "Linux/"
                print("Running command:", " ".join(args_pkg), "; working_dir=" + working_dir)
                proc = subprocess.run(args_pkg, cwd=working_dir)
                if proc.returncode != 0:
                    print("Failed to create debian package for CLI")
                    exit(1)

                relative_path = r"/Linux/" + get_out_arch_name(target) + "/"
                package_os_dir = new_build_dir + relative_path
                mkdir_if_not_exists(package_os_dir)

                file_from = os.path.join("Linux", "uptooda-cli_{version_clean}.{build_number}_{arch}.deb".format(
                    version_clean=version_header_defines["IU_APP_VER_CLEAN"], build_number=build_number, arch=target.get("deb_package_arch"))
                )
                filename = "uptooda-cli_" + app_ver + "-build-" + build_number + "_" + target.get("deb_package_arch") + ".deb"
                file_to = package_os_dir + filename
                print("Copy file from:", file_from)
                print("Copy file to:", file_to)
                shutil.copyfile(file_from, file_to)
                json_data = add_output_file(json_data, target, json_file_path, "Debian package", file_to, relative_path + filename, APP_NAME + " (CLI)")

                file_from = os.path.join("Linux", "uptooda-cli-{version_clean}.{build_number}-{arch}.tar.xz".format(
                    version_clean=version_header_defines["IU_APP_VER_CLEAN"], build_number=build_number, arch=target.get("deb_package_arch"))
                )
                filename = "uptooda-cli_" + app_ver + "-build-" + build_number + "_" + target.get("deb_package_arch") + ".tar.xz"
                file_to = package_os_dir + filename
                print("Copy file from:", file_from)
                print("Copy file to:", file_to)
                shutil.copyfile(file_from, file_to)
                json_data = add_output_file(json_data, target, json_file_path, ".tar.xz archive", file_to, relative_path + filename, APP_NAME + " (CLI)")

                if target.get("build_qt_gui"):
                    args_gui = _linux_command(["/bin/bash", "create-qimageuploader-package.sh", target.get("deb_package_arch"), target.get("objcopy")])
                    working_dir = repo_dir_abs + used_dist_dir + "Linux/"
                    print("Running command:", " ".join(args_gui), "; working_dir=" + working_dir)
                    proc = subprocess.run(args_gui, cwd=working_dir)
                    if proc.returncode != 0:
                        print("Failed to create debian package for Qt GUI")
                        exit(1)

                    file_from = os.path.join("Linux", "uptooda_{version_clean}.{build_number}_{arch}.deb".format(
                        version_clean=version_header_defines["IU_APP_VER_CLEAN"], build_number=build_number, arch=target.get("deb_package_arch"))
                    )
                    filename = "uptooda_" + app_ver + "-build-" + build_number + "_" + target.get("deb_package_arch") + ".deb"
                    file_to = package_os_dir + filename
                    print("Copy file from:", file_from)
                    print("Copy file to:", file_to)
                    shutil.copyfile(file_from, file_to)
                    json_data = add_output_file(json_data, target, json_file_path, "Debian package", file_to, relative_path + filename, APP_NAME + " (Qt GUI)")

        print("Target finished successfully:", target_full_name)

    write_builds_file(json_builds_info_file, version_header_defines)
    print("\nFinish.")


if __name__ == "__main__":
    main()
