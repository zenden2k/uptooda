#Requires -Version 5.1
<#
.SYNOPSIS
    Bootstrap script for Uptooda build environment (Windows + WSL2).

.DESCRIPTION
    Checks and installs all build dependencies on Windows and inside WSL2.
    Run from a PowerShell terminal with internet access.
    Visual Studio must be installed manually (see below).

.PARAMETER CheckOnly
    Only check dependencies, do not install anything.

.PARAMETER SkipWsl
    Skip WSL2 dependency check/install.

.PARAMETER SkipWindows
    Skip Windows dependency check/install.

.EXAMPLE
    .\bootstrap.ps1
    .\bootstrap.ps1 -CheckOnly
    .\bootstrap.ps1 -SkipWsl
#>

param(
    [switch]$CheckOnly,
    [switch]$SkipWsl,
    [switch]$SkipWindows
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

$script:HasErrors   = $false
$script:HasWarnings = $false

function Write-Header([string]$text) {
    Write-Host ""
    Write-Host "=== $text ===" -ForegroundColor Cyan
}

function Write-Ok([string]$text) {
    Write-Host "  [OK]   $text" -ForegroundColor Green
}

function Write-Fail([string]$text) {
    Write-Host "  [FAIL] $text" -ForegroundColor Red
    $script:HasErrors = $true
}

function Write-Warn([string]$text) {
    Write-Host "  [WARN] $text" -ForegroundColor Yellow
    $script:HasWarnings = $true
}

function Write-Info([string]$text) {
    Write-Host "         $text" -ForegroundColor Gray
}

function Test-Command([string]$cmd) {
    return [bool](Get-Command $cmd -ErrorAction SilentlyContinue)
}

function Invoke-Winget([string]$packageId, [string]$displayName) {
    Write-Info "Installing $displayName via winget..."
    winget install --id $packageId --silent --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) {
        Write-Warn "winget exited with code $LASTEXITCODE for $displayName -- check manually."
    }
}

function Refresh-Path {
    $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "Machine") + ";" +
                [System.Environment]::GetEnvironmentVariable("PATH", "User")
}

function Test-PathContainsDirectory([string]$directory) {
    if (-not $directory -or -not (Test-Path $directory -PathType Container)) { return $false }
    $fullDirectory = [System.IO.Path]::GetFullPath($directory).TrimEnd('\')
    $pathEntries = ($env:PATH -split ';') | Where-Object { $_.Trim() -ne "" }
    foreach ($entry in $pathEntries) {
        try {
            $fullEntry = [System.IO.Path]::GetFullPath($entry).TrimEnd('\')
            if ([string]::Equals($fullEntry, $fullDirectory, [System.StringComparison]::OrdinalIgnoreCase)) {
                return $true
            }
        } catch {}
    }
    return $false
}

function Add-PathSuggestion([System.Collections.ArrayList]$suggestions, [string]$directory) {
    if (-not $directory -or -not (Test-Path $directory -PathType Container)) { return }
    $fullDirectory = [System.IO.Path]::GetFullPath($directory).TrimEnd('\')
    if (Test-PathContainsDirectory $fullDirectory) { return }
    foreach ($suggestion in $suggestions) {
        if ([string]::Equals($suggestion, $fullDirectory, [System.StringComparison]::OrdinalIgnoreCase)) {
            return
        }
    }
    [void]$suggestions.Add($fullDirectory)
}

function Get-WindowsPathSuggestions {
    $suggestions = New-Object System.Collections.ArrayList

    if (-not (Test-Command "cmake")) {
        Add-PathSuggestion $suggestions (Join-Path $env:ProgramFiles "CMake\bin")
    }
    if (-not (Test-Command "git")) {
        Add-PathSuggestion $suggestions (Join-Path $env:ProgramFiles "Git\cmd")
    }
    if (-not (Test-Command "7z")) {
        Add-PathSuggestion $suggestions (Join-Path $env:ProgramFiles "7-Zip")
        Add-PathSuggestion $suggestions (Join-Path ${env:ProgramFiles(x86)} "7-Zip")
    }
    if (-not (Test-Command "iscc")) {
        Add-PathSuggestion $suggestions (Join-Path ${env:ProgramFiles(x86)} "Inno Setup 6")
        Add-PathSuggestion $suggestions (Join-Path $env:ProgramFiles "Inno Setup 6")
        Add-PathSuggestion $suggestions (Join-Path $env:LocalAppData "Programs\Inno Setup 6")
    }

    if (-not (Test-Command "python") -or -not (Test-Command "pip") -or -not (Test-Command "conan")) {
        $pythonRoots = @(
            Join-Path $env:LocalAppData "Programs\Python"
            Join-Path $env:AppData "Python"
        ) | Where-Object { $_ -and (Test-Path $_ -PathType Container) }
        foreach ($root in $pythonRoots) {
            Get-ChildItem -Path $root -Directory -Filter "Python*" -ErrorAction SilentlyContinue | ForEach-Object {
                Add-PathSuggestion $suggestions $_.FullName
                Add-PathSuggestion $suggestions (Join-Path $_.FullName "Scripts")
            }
        }
    }

    if (-not (Test-MsBuild)) {
        $vswhere = Find-VsWhere
        if ($vswhere) {
            $vsPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath 2>$null
            if ($vsPath) {
                Add-PathSuggestion $suggestions (Join-Path $vsPath "MSBuild\Current\Bin")
            }
        }
    }

    return @($suggestions)
}

function Write-WindowsPathSuggestions {
    if ($SkipWindows) { return }

    $suggestions = Get-WindowsPathSuggestions
    Write-Host ""
    Write-Host "Windows PATH additions:" -ForegroundColor Cyan
    if ($suggestions.Count -eq 0) {
        Write-Host "  No additional existing directories detected." -ForegroundColor Green
        return
    }

    Write-Host "  Add these directories to Windows PATH if the corresponding tools are not found in a new terminal:" -ForegroundColor Gray
    foreach ($directory in $suggestions) {
        Write-Host "  $directory"
    }
}

# ---------------------------------------------------------------------------
# Visual Studio detection
# ---------------------------------------------------------------------------

function Find-VsWhere {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) { return $vswhere }
    $vswhere = "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) { return $vswhere }
    return $null
}

function Test-VisualStudio {
    $vswhere = Find-VsWhere
    if (-not $vswhere) { return $false }

    $vsPath = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath 2>$null

    return ($null -ne $vsPath -and $vsPath -ne "")
}

function Test-MsBuild {
    # msbuild is available when running inside VS Developer Command Prompt,
    # or when added to PATH by VS installer.
    return (Test-Command "msbuild")
}

# ---------------------------------------------------------------------------
# Windows dependency checks / installs
# ---------------------------------------------------------------------------

function Check-Winget {
    if (-not (Test-Command "winget")) {
        Write-Fail "winget not found. Install 'App Installer' from the Microsoft Store."
        return $false
    }
    Write-Ok "winget"
    return $true
}

function Check-Git {
    if (Test-Command "git") {
        $ver = (git --version 2>&1)
        Write-Ok "git  ($ver)"
        return $true
    }
    if ($CheckOnly) { Write-Fail "git not found"; return $false }
    Invoke-Winget "Git.Git" "Git"
    Refresh-Path
    if (Test-Command "git") { Write-Ok "git  (installed)"; return $true }
    Write-Fail "git -- installation may require a terminal restart"
    return $false
}

function Check-Cmake-Windows {
    if (Test-Command "cmake") {
        $ver = (cmake --version 2>&1 | Select-Object -First 1)
        Write-Ok "cmake  ($ver)"
        return $true
    }
    if ($CheckOnly) { Write-Fail "cmake not found"; return $false }
    Invoke-Winget "Kitware.CMake" "CMake"
    Refresh-Path
    if (Test-Command "cmake") { Write-Ok "cmake  (installed)"; return $true }
    Write-Fail "cmake -- installation may require a terminal restart"
    return $false
}

function Check-Conan-Windows {
    if (Test-Command "conan") {
        $raw = (conan --version 2>&1)
        # Must be Conan 2.x
        if ($raw -match "Conan version (\d+)\.") {
            $major = [int]$Matches[1]
            if ($major -ge 2) {
                Write-Ok "conan  ($raw)"
                return $true
            }
            Write-Fail "conan -- found version $raw, but Conan 2.x is required"
            Write-Info "Run: pip install --upgrade conan"
            return $false
        }
        Write-Warn "conan -- could not determine version ($raw)"
        return $false
    }
    if ($CheckOnly) { Write-Fail "conan not found  ->  pip install conan"; return $false }
    Write-Info "Installing conan via pip..."
    pip install conan
    Refresh-Path
    if (Test-Command "conan") { Write-Ok "conan  (installed)"; return $true }
    Write-Fail "conan -- install failed"
    return $false
}

function Check-Python {
    if (Test-Command "python") {
        $ver = (python --version 2>&1)
        Write-Ok "python  ($ver)"
        return $true
    }
    if ($CheckOnly) { Write-Fail "python not found"; return $false }
    Invoke-Winget "Python.Python.3" "Python 3"
    Refresh-Path
    if (Test-Command "python") { Write-Ok "python  (installed)"; return $true }
    Write-Fail "python -- installation may require a terminal restart"
    return $false
}

function Check-InnoSetup {
    try {
        $out = (iscc /? 2>&1) -join " "
        if ($out -match "Inno Setup") {
            Write-Ok "Inno Setup (iscc)"
            return $true
        }
    } catch {}

    # Try common install paths
    $candidates = @(
        "${env:ProgramFiles(x86)}\Inno Setup 6\iscc.exe",
        "${env:ProgramFiles}\Inno Setup 6\iscc.exe"
    )
    foreach ($path in $candidates) {
        if (Test-Path $path) {
            Write-Warn "Inno Setup found at '$path' but not in PATH."
            Write-Info "Add '$([System.IO.Path]::GetDirectoryName($path))' to your PATH."
            return $false
        }
    }

    if ($CheckOnly) {
        Write-Fail "Inno Setup (iscc) not found  ->  https://jrsoftware.org/isinfo.php"
        return $false
    }
    Invoke-Winget "JRSoftware.InnoSetup" "Inno Setup"
    Refresh-Path
    try {
        $out = (iscc /? 2>&1) -join " "
        if ($out -match "Inno Setup") { Write-Ok "Inno Setup  (installed)"; return $true }
    } catch {}
    Write-Warn "Inno Setup installed -- add its directory to PATH and restart terminal"
    return $false
}

function Check-7zip {
    if (Test-Command "7z") {
        Write-Ok "7-Zip (7z)"
        return $true
    }
    if ($CheckOnly) {
        Write-Fail "7-Zip (7z) not found  ->  https://www.7-zip.org"
        return $false
    }
    Invoke-Winget "7zip.7zip" "7-Zip"
    Refresh-Path
    if (Test-Command "7z") { Write-Ok "7-Zip  (installed)"; return $true }
    Write-Warn "7-Zip installed -- add its directory to PATH and restart terminal"
    return $false
}

function Check-VisualStudio {
    if (Test-VisualStudio) {
        Write-Ok "Visual Studio (with C++ Desktop workload)"
    } else {
        Write-Fail "Visual Studio with 'Desktop development with C++' workload not found."
        Write-Info "Download: https://visualstudio.microsoft.com/downloads/"
        Write-Info "Required workload: 'Desktop development with C++'"
        Write-Info "(includes MSVC, MSBuild, Windows SDK)"
    }
}

function Check-MsBuild {
    if (Test-MsBuild) {
        $ver = (msbuild --version 2>&1 | Select-Object -First 1)
        Write-Ok "msbuild  ($ver)"
    } else {
        Write-Warn "msbuild not in PATH."
        Write-Info "Either run this script from a Visual Studio Developer Command Prompt,"
        Write-Info "or add MSBuild to your PATH after installing Visual Studio."
    }
}

# ---------------------------------------------------------------------------
# WSL2 checks / installs
# ---------------------------------------------------------------------------

function Test-Wsl {
    if (-not (Test-Command "wsl")) { return $false }
    $oldErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $list = (wsl --list --quiet 2>&1)
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $oldErrorActionPreference
    }
    # wsl --status returns 0 even when WSL is not fully set up on older builds;
    # a running distro is the real test.
    if ($exitCode -ne 0) { return $false }
    $listText = ($list -join " ").Trim()
    return ($listText -ne "" -and $listText -notmatch "Wsl/|E_ACCESSDENIED")
}

function ConvertTo-WslPath([string]$windowsPath) {
    if ($windowsPath -match "^([A-Za-z]):\\(.*)$") {
        $drive = $Matches[1].ToLowerInvariant()
        $path = $Matches[2] -replace "\\", "/"
        return "/mnt/$drive/$path"
    }

    Write-Fail "Unable to convert path for WSL: $windowsPath"
    Write-Info "Only local drive paths like C:\path\to\file are supported."
    return $null
}

function Test-WslFile([string]$wslPath) {
    $oldErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        wsl -- test -f $wslPath
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $oldErrorActionPreference
    }
    if ($exitCode -ne 0) {
        Write-Fail "WSL cannot access bootstrap script: $wslPath"
        return $null
    }
    return $true
}

function Invoke-WslBootstrap {
    $scriptPath = Join-Path $PSScriptRoot "bootstrap-wsl.sh"
    if (-not (Test-Path $scriptPath)) {
        Write-Fail "WSL bootstrap script not found: $scriptPath"
        return $false
    }

    $wslScriptPath = ConvertTo-WslPath (Resolve-Path $scriptPath).Path
    if (-not $wslScriptPath) { return $false }
    if (-not (Test-WslFile $wslScriptPath)) { return $false }

    $scriptArgs = @()
    if ($CheckOnly) { $scriptArgs += "--check-only" }

    $oldErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        wsl -- bash $wslScriptPath @scriptArgs
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $oldErrorActionPreference
    }

    if ($exitCode -ne 0) {
        Write-Fail "WSL dependency bootstrap failed."
        return $false
    }
    return $true
}

function Check-WslAvailable {
    if (-not (Test-Command "wsl")) {
        Write-Fail "WSL not found. Enable with: wsl --install"
        return $false
    }
    $oldErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $wslList = wsl --list --quiet 2>&1
        $wslListExitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $oldErrorActionPreference
    }
    $wslListText = ($wslList -join " ").Trim()
    if ($wslListExitCode -ne 0 -or $wslListText -match "Wsl/|E_ACCESSDENIED") {
        Write-Fail "Unable to enumerate WSL distributions."
        Write-Info $wslListText
        return $false
    }
    $distros = $wslList | Where-Object { $_.Trim() -ne "" }
    if (-not $distros) {
        Write-Fail "WSL is installed but no Linux distribution found."
        Write-Info "Run: wsl --install -d Ubuntu"
        return $false
    }
    Write-Ok "WSL2  (distros: $($distros -join ', '))"
    return $true
}

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

Write-Host ""
Write-Host "Uptooda build environment bootstrap" -ForegroundColor White
Write-Host "Mode: $(if ($CheckOnly) { 'check only' } else { 'check + install' })"

# ---- Windows ---------------------------------------------------------------
if (-not $SkipWindows) {
    Write-Header "Windows dependencies"

    $wingetOk = Check-Winget

    [void](Check-Git)
    [void](Check-Python)
    [void](Check-Cmake-Windows)

    if ($wingetOk -or (Test-Command "pip")) {
        [void](Check-Conan-Windows)
    } else {
        Write-Warn "Skipping conan check -- pip not available yet (install Python first)"
    }

    [void](Check-7zip)
    [void](Check-InnoSetup)
    Check-VisualStudio
    Check-MsBuild
}

# ---- WSL2 ------------------------------------------------------------------
if (-not $SkipWsl) {
    Write-Header "WSL2 dependencies"

    $wslOk = Check-WslAvailable
    if ($wslOk) {
        [void](Invoke-WslBootstrap)
    } else {
        Write-Warn "Skipping WSL dependency checks -- WSL2 not available."
    }
}

Write-WindowsPathSuggestions

# ---- Summary ---------------------------------------------------------------
Write-Host ""
Write-Host "==============================" -ForegroundColor Cyan
if ($script:HasErrors) {
    Write-Host "Result: some dependencies are MISSING." -ForegroundColor Red
    Write-Host "Fix the issues marked [FAIL] above and re-run this script."
    exit 1
} elseif ($script:HasWarnings) {
    Write-Host "Result: OK with warnings." -ForegroundColor Yellow
    Write-Host "Review [WARN] items above -- a terminal restart may be needed."
    exit 0
} else {
    Write-Host "Result: all dependencies satisfied." -ForegroundColor Green
    exit 0
}
