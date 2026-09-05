#!/usr/bin/env bash
# Configure, build and optionally test AERIFORM (Git Bash on Windows, macOS, Linux).
#   scripts/build.sh                 # Release
#   scripts/build.sh Debug
#   scripts/build.sh Release --test  # build, then run unit + smoke tests
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG="${1:-Release}"
RUN_TESTS=0; [[ "${2:-}" == "--test" ]] && RUN_TESTS=1
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    TOOLCHAIN="${AERIFORM_TOOLCHAIN:-/d/dev/tools/mingw64/bin}"
    [[ -d "$TOOLCHAIN" ]] && export PATH="$TOOLCHAIN:$PATH"
    PRESET="mingw-release"; [[ "$CONFIG" == "Debug" ]] && PRESET="mingw-debug" ;;
  *) PRESET="unix-release" ;;
esac
cd "$ROOT"
# Ninja post-build steps break on build paths containing '^' (cmd.exe escape char):
# build outside the source tree in that case (override with AERIFORM_BUILD_ROOT).
if [[ -n "${AERIFORM_BUILD_ROOT:-}" ]]; then BUILD_ROOT="$AERIFORM_BUILD_ROOT"
elif [[ "$ROOT" == *"^"* ]]; then BUILD_ROOT="/d/dev/build/aeriform"
else BUILD_ROOT="$ROOT/build"; fi
BUILD_DIR="$BUILD_ROOT/$PRESET"
cmake --preset "$PRESET" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --parallel
if [[ $RUN_TESTS == 1 ]]; then
  "$BUILD_DIR/AeriformTests" && "$BUILD_DIR/AeriformTests" --smoke
fi
echo "Artefacts: $BUILD_DIR/Aeriform_artefacts/$CONFIG/"
