#!/usr/bin/env bash
set -u

CHECK_ONLY=0
if [[ "${1:-}" == "--check-only" ]]; then
    CHECK_ONLY=1
fi

HAS_ERRORS=0
LOCAL_BIN="$HOME/.local/bin"
CONAN_VENV="$HOME/.local/share/uptooda-conan"
export PATH="$LOCAL_BIN:$PATH"

ok() {
    printf '  [OK]   %s\n' "$1"
}

fail() {
    printf '  [FAIL] %s\n' "$1"
    HAS_ERRORS=1
}

info() {
    printf '         %s\n' "$1"
}

version_line() {
    "$@" 2>&1 | head -n 1
}

check_command() {
    local name="$1"
    local command_name="$2"
    shift 2

    if command -v "$command_name" >/dev/null 2>&1; then
        ok "WSL $name  ($(version_line "$@"))"
        return 0
    fi

    fail "WSL $name not found"
    return 1
}

APT_PACKAGES=(
    git
    cmake
    ninja-build
    gcc
    g++
    make
    doxygen
    gettext
    binutils
    binutils-aarch64-linux-gnu
    perl
    python3-pip
    python3-venv
)

if [[ "$CHECK_ONLY" -eq 0 ]]; then
    if [[ "${CI:-}" == "true" || "${GITHUB_ACTIONS:-}" == "true" ]]; then
        info "Checking passwordless sudo for WSL dependency installation..."
        if ! sudo -n -v; then
            fail "passwordless sudo is required in CI"
            info "Configure the runner for passwordless sudo or install WSL dependencies before running this script."
            exit 1
        fi
        SUDO_CMD=(sudo -n)
    else
        info "Requesting sudo once for WSL dependency installation..."
        if ! sudo -v; then
            fail "sudo authentication failed"
            exit 1
        fi
        SUDO_CMD=(sudo)
    fi

    if ! "${SUDO_CMD[@]}" true; then
        fail "sudo authentication failed"
        exit 1
    fi

    while true; do
        "${SUDO_CMD[@]}" true
        sleep 60
        kill -0 "$$" >/dev/null 2>&1 || exit
    done 2>/dev/null &
    SUDO_KEEPALIVE_PID=$!
    trap 'kill "$SUDO_KEEPALIVE_PID" >/dev/null 2>&1 || true' EXIT

    info "Updating WSL apt package index..."
    "${SUDO_CMD[@]}" apt-get update

    info "Installing WSL apt packages..."
    "${SUDO_CMD[@]}" apt-get install -y "${APT_PACKAGES[@]}"
fi

check_command "git" "git" git --version
check_command "cmake" "cmake" cmake --version
check_command "ninja" "ninja" ninja --version
check_command "gcc" "gcc" gcc --version
check_command "make" "make" make --version
check_command "perl" "perl" perl --version
check_command "doxygen" "doxygen" doxygen --version
check_command "msgfmt" "msgfmt" msgfmt --version
check_command "objcopy" "objcopy" objcopy --version
check_command "aarch64-linux-gnu-objcopy" "aarch64-linux-gnu-objcopy" aarch64-linux-gnu-objcopy --version

CONAN_VERSION=""
if command -v conan >/dev/null 2>&1; then
    CONAN_VERSION="$(version_line conan --version)"
elif python3 -m conan --version >/dev/null 2>&1; then
    CONAN_VERSION="$(version_line python3 -m conan --version)"
fi

if [[ "$CONAN_VERSION" =~ Conan\ version\ ([0-9]+)\. ]] && [[ "${BASH_REMATCH[1]}" -ge 2 ]]; then
    ok "WSL conan  ($CONAN_VERSION)"
else
    if [[ "$CHECK_ONLY" -eq 1 ]]; then
        fail "WSL conan not found  ->  run this script without --check-only"
    else
        info "Installing WSL conan into $CONAN_VENV..."
        mkdir -p "$LOCAL_BIN"
        python3 -m venv "$CONAN_VENV"
        "$CONAN_VENV/bin/python" -m pip install --upgrade pip
        "$CONAN_VENV/bin/python" -m pip install --upgrade conan
        ln -sf "$CONAN_VENV/bin/conan" "$LOCAL_BIN/conan"
        hash -r

        if command -v conan >/dev/null 2>&1; then
            CONAN_VERSION="$(version_line conan --version)"
        elif python3 -m conan --version >/dev/null 2>&1; then
            CONAN_VERSION="$(version_line python3 -m conan --version)"
        fi

        if [[ "$CONAN_VERSION" =~ Conan\ version\ ([0-9]+)\. ]] && [[ "${BASH_REMATCH[1]}" -ge 2 ]]; then
            ok "WSL conan  ($CONAN_VERSION)"
            if [[ ":$PATH:" != *":$LOCAL_BIN:"* ]]; then
                info "Add $LOCAL_BIN to WSL PATH to run conan directly."
            fi
        else
            fail "WSL conan -- install failed"
        fi
    fi
fi

exit "$HAS_ERRORS"
