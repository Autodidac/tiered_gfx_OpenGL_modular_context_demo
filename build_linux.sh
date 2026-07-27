#!/usr/bin/env bash
set -euo pipefail

preset="${1:-linux-gcc-release}"
if [[ "$preset" != "linux-gcc-release" ]]; then
  echo "Supported verified Linux preset: linux-gcc-release" >&2
  exit 2
fi

if command -v g++-14 >/dev/null 2>&1; then
  compiler="$(command -v g++-14)"
elif command -v g++ >/dev/null 2>&1; then
  compiler="$(command -v g++)"
  major="$($compiler -dumpfullversion -dumpversion | cut -d. -f1)"
  if [[ -z "$major" || "$major" -lt 14 ]]; then
    echo "GCC 14 or newer is required; found $($compiler --version | head -n1)." >&2
    exit 2
  fi
else
  echo "GCC 14 or newer was not found. Install g++-14." >&2
  exit 2
fi

cmake --preset "$preset" -DCMAKE_CXX_COMPILER="$compiler"
# GCC 14 can corrupt a named-module BMI when multiple consumers load it
# concurrently. Keep the verified GCC module build serialized until that
# compiler defect is resolved; MSVC builds remain parallel.
cmake --build --preset "$preset" --parallel 1
python3 tools/validate_scene_defaults.py
printf 'Built: build/linux-gcc/bin/Release/epoch_integrated_opengl_scene\n'
