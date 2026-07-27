# 4.6.7

This release repairs the model-packaging regression in 4.6.6 and publishes actual desktop binaries.

## Fixed

- Restored the complete set of 24 authored OBJ models from the known-good 4.6.6 package.
- Removed the global missing-model cube fallback that caused trees, props, primitives, diagnostics, terrain, and character models to appear as boxes.
- Missing or damaged model data now fails validation and configuration instead of silently changing scene geometry.
- The model archive is reconstructed from twelve exact text-safe chunks and verified against SHA-256 `5026d93ee4514264da5948c7c5b663e0f2890de2f7e0cdc2bbfaf2da8ac5c4fd`.
- Runtime packages are checked for restored tree models before publication.
- The Linux smoke test rejects any diagnostic cube-fallback message.

## Binary downloads

- `epoch_integrated_opengl_scene_4_6_7_windows_x64.zip` — ready-to-run Windows executable and assets.
- `epoch_integrated_opengl_scene_4_6_7_linux_x64.tar.gz` — ready-to-run Linux x64 executable and assets for Ubuntu 24.04-compatible systems.
- `epoch_integrated_opengl_scene_4_6_7_source.zip` — source package for developers.

## Validation

- Windows Visual Studio 2022 configure and Release build.
- Linux GCC 14 C++23 named-module configure and Release build.
- Linux Xvfb startup smoke test.
- 269 contiguous factory scene records, no deleted tombstones, and UV scale `1.0`.
- Exact archive checksum and all 24 required model paths.
