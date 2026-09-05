<#
.SYNOPSIS
  Configure, build and (optionally) test AERIFORM on Windows with the portable
  MinGW-w64 toolchain.

  powershell -ExecutionPolicy Bypass -File scripts\build.ps1               # Release build
  powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Config Debug
  powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Test         # build + run tests
  powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Clean        # wipe build dir first

  Set AERIFORM_TOOLCHAIN to the directory containing g++.exe / ninja.exe / cmake.exe
  if the toolchain is not at D:\dev\tools\mingw64\bin.
#>
param(
    [ValidateSet("Release","Debug")] [string]$Config = "Release",
    [switch]$Test,
    [switch]$Clean,
    [string]$Toolchain = $(if ($env:AERIFORM_TOOLCHAIN) { $env:AERIFORM_TOOLCHAIN } else { "D:\dev\tools\mingw64\bin" })
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (Test-Path $Toolchain) { $env:PATH = "$Toolchain;" + $env:PATH }
$preset = if ($Config -eq "Debug") { "mingw-debug" } else { "mingw-release" }
# CMake's Ninja generator wraps post-build steps in cmd.exe "cd /D <dir>" calls, which
# break when the build directory path contains '^'. Build elsewhere in that case.
$buildRoot = if ($env:AERIFORM_BUILD_ROOT) { $env:AERIFORM_BUILD_ROOT }
             elseif ($root.Contains("^")) { "D:\dev\build\aeriform" }
             else { Join-Path $root "build" }
$buildDir = Join-Path $buildRoot $preset
if ($Clean -and (Test-Path $buildDir)) { Remove-Item -Recurse -Force $buildDir }
Push-Location $root
try {
    cmake --preset $preset -B "$buildDir"
    if ($LASTEXITCODE -ne 0) { throw "configure failed" }
    cmake --build "$buildDir" --parallel
    if ($LASTEXITCODE -ne 0) { throw "build failed" }
    if ($Test) {
        & (Join-Path $buildDir "AeriformTests.exe")
        if ($LASTEXITCODE -ne 0) { throw "tests failed" }
        & (Join-Path $buildDir "AeriformTests.exe") --smoke
        if ($LASTEXITCODE -ne 0) { throw "smoke test failed" }
    }
    Write-Host ""
    Write-Host "VST3:       $buildDir\Aeriform_artefacts\$Config\VST3\AERIFORM.vst3"
    Write-Host "Standalone: $buildDir\Aeriform_artefacts\$Config\Standalone\AERIFORM.exe"
} finally { Pop-Location }
