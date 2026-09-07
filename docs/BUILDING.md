# Building AERIFORM

## Prerequisites

| Tool | Version used | Notes |
|---|---|---|
| CMake | 3.22+ (3.31 bundled with the toolchain below) | |
| Ninja | 1.12 | bundled with the toolchain below |
| C++20 compiler | **GCC 14.2 MinGW-w64 (WinLibs, POSIX threads, UCRT)** or Visual Studio 2022 | |
| Git | any | to fetch JUCE if `external/JUCE` is absent |
| Python 3 | any | only to regenerate the parameter table (`scripts/gen_params.py`) |

No admin rights are required for the MinGW path. JUCE 7.0.12 is vendored in
`external/JUCE` (shallow clone). If the folder is missing, CMake fetches it
automatically with `FetchContent`.

## Toolchain setup (portable, no installer)

1. Download `winlibs-x86_64-posix-seh-gcc-14.2.0-mingw-w64ucrt-12.0.0-r3.zip`
   from <https://github.com/brechtsanders/winlibs_mingw/releases>.
2. Extract it so that `g++.exe`, `ninja.exe` and `cmake.exe` are in
   `D:\dev\tools\mingw64\bin` (any folder works: set the environment variable
   `AERIFORM_TOOLCHAIN` to that `bin` directory).

## Build (exact commands)

From the repository root, in PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1            # Release build
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Test      # build + unit + smoke tests
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Config Debug
```

or from Git Bash:

```bash
scripts/build.sh                 # Release
scripts/build.sh Release --test  # build + tests
```

Equivalent manual commands:

```powershell
$env:PATH = "D:\dev\tools\mingw64\bin;" + $env:PATH
cmake --preset mingw-release -B D:\dev\build\aeriform\mingw-release
cmake --build D:\dev\build\aeriform\mingw-release --parallel
```

**Build directory location.** CMake's Ninja generator wraps JUCE's post-build
steps in `cmd.exe /C "cd /D <dir> && ..."`, which breaks when the path contains
`^` (cmd's escape character). The scripts therefore build under
`D:\dev\build\aeriform` when the source path contains `^`, and under `build\`
inside the repository otherwise. Override with `AERIFORM_BUILD_ROOT`.

**Parameter table.** `Source/Params/ParamIDs.h` and `ParamTable.inc` are
generated from `scripts/gen_params.py` (the single source of truth for IDs,
ranges, defaults, units and tooltips). After editing the script run
`python scripts/gen_params.py` and rebuild; the parameter tests verify the
generated table.

## Visual Studio 2022

```powershell
cmake --preset msvc
cmake --build --preset msvc-release
```

(Configured but not exercised on the development machine, which has no Visual
Studio installed.)

## Build outputs

| Artefact | Path (Release, MinGW preset) |
|---|---|
| VST3 bundle | `<build>/Aeriform_artefacts/Release/VST3/AERIFORM.vst3/` |
| Standalone | `<build>/Aeriform_artefacts/Release/Standalone/AERIFORM.exe` |
| Tests | `<build>/AeriformTests.exe` |
| Host checker | `<build>/AeriformHostCheck_artefacts/Release/AeriformHostCheck.exe` |

The Windows binaries are statically linked against the compiler runtime
(`-static -static-libgcc -static-libstdc++`), so the VST3 depends only on
system DLLs and the Universal CRT that ships with Windows 10 / 11.

## macOS and Linux

The project is generator-agnostic. Untested on those platforms here, but the
intended commands are:

```bash
cmake --preset unix-release          # Ninja, Release
cmake --build --preset unix-release
```

- macOS: Xcode command-line tools; the `AU` format is added automatically.
- Linux: install the usual JUCE dependencies (`libasound2-dev libjack-jackd2-dev
  libfreetype6-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev
  libcurl4-openssl-dev libgl1-mesa-dev`), then the commands above.

## Installing and running

**VST3 (Windows):** copy the whole `AERIFORM.vst3` folder to
`C:\Program Files\Common Files\VST3\` (or configure `-DAERIFORM_COPY_PLUGIN=ON`
to copy after every build). Rescan plug-ins in your DAW; AERIFORM appears as an
instrument by "Aeriform Audio".

**Standalone:** run `AERIFORM.exe`. Use *Options -> Audio/MIDI Settings* to pick
the audio device, sample rate, buffer size and MIDI input. The standalone mutes
the audio input by default (feedback protection): untick *Mute audio input* to
blow into a microphone and let it drive the resonators (see the sidechain
section of [the guide](GUIDE.md)).

---
Back to [README](../README.md).
