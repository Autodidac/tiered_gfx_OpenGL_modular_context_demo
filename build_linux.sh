#!/usr/bin/env bash
set -euo pipefail

preset="${1:-linux-gcc-release}"
if [[ "$preset" != "linux-gcc-release" ]]; then
  echo "Supported verified Linux preset: linux-gcc-release" >&2
  echo "Clang can use the same modules when a matching clang-scan-deps is installed; configure it manually." >&2
  exit 2
fi

cmake --preset "$preset"
cmake --build --preset "$preset"
printf 'Built: build/linux-gcc/bin/Release/epoch_integrated_opengl_scene\n'
