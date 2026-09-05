<#
.SYNOPSIS
    One-shot configure + build (+ optional run) for MiniGame on Windows with MinGW-w64.

.DESCRIPTION
    Expects a toolchain folder laid out as:
        <ToolchainDir>\mingw64\bin\g++.exe        (WinLibs GCC 13.1.0, MSVCRT, posix-seh)
        <ToolchainDir>\SFML-2.6.1\lib\cmake\SFML  (SFML 2.6.1 "GCC 13.1.0 MinGW 64-bit" package)
    CMake + Ninja must be on PATH (e.g. `pip install cmake ninja`).

.EXAMPLE
    .\build.ps1                 # Release build into .\build
    .\build.ps1 -Run            # build then launch the game
    .\build.ps1 -Test           # build then run the unit tests (ctest)
    .\build.ps1 -Config Debug
    .\build.ps1 -ToolchainDir E:\toolchain
#>
param(
    [string]$ToolchainDir = (Join-Path (Split-Path $PSScriptRoot -Parent) "toolchain"),
    [ValidateSet("Release", "Debug")][string]$Config = "Release",
    [string]$BuildDir = "build",
    [switch]$Run,
    [switch]$Test,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

$mingwBin = Join-Path $ToolchainDir "mingw64\bin"
$sfmlDir  = Join-Path $ToolchainDir "SFML-2.6.1\lib\cmake\SFML"

if (-not (Test-Path (Join-Path $mingwBin "g++.exe"))) { throw "g++ not found under $mingwBin" }
if (-not (Test-Path $sfmlDir)) { throw "SFML cmake config not found under $sfmlDir" }
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { throw "cmake not on PATH (pip install cmake ninja)" }

# Append (not prepend): WinLibs ships its own older cmake.exe that must not shadow the one on PATH.
$env:PATH = "$env:PATH;$mingwBin"
$cxx = (Join-Path $mingwBin "g++.exe") -replace '\\', '/'
$sfmlDirFwd = $sfmlDir -replace '\\', '/'

if ($Clean -and (Test-Path $BuildDir)) { Remove-Item -Recurse -Force $BuildDir }

cmake -S . -B $BuildDir -G Ninja "-DCMAKE_BUILD_TYPE=$Config" "-DCMAKE_CXX_COMPILER=$cxx" "-DSFML_DIR=$sfmlDirFwd"
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

cmake --build $BuildDir
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host "`nBuilt: $(Resolve-Path (Join-Path $BuildDir 'MiniGame.exe'))" -ForegroundColor Green

if ($Test) {
    ctest --test-dir $BuildDir --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Tests failed" }
}

if ($Run) {
    Push-Location $BuildDir
    try { & .\MiniGame.exe } finally { Pop-Location }
}
