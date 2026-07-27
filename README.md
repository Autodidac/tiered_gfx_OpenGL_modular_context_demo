# Tiered OpenGL Modular Context Demo

A C++23 named-module renderer and scene editor demonstrating a practical **Tier 0 / Tier 1 graphics architecture** on one authored OpenGL scene.

The project is a compact reference for cross-platform context creation, explicit renderer ownership, metallic/roughness PBR, editable scene data, and capability-budgeted rendering paths.

## Download and run

GitHub Releases provide three separate packages:

- **Windows x64 binary** — `epoch_integrated_opengl_scene_4_6_7_windows_x64.zip`
- **Linux x64 binary** — `epoch_integrated_opengl_scene_4_6_7_linux_x64.tar.gz`
- **Source archive** — `epoch_integrated_opengl_scene_4_6_7_source.zip`

The Windows and Linux packages contain the compiled executable and the complete runtime `assets` directory. They do not require compiling the project.

Windows: extract the ZIP and run `epoch_integrated_opengl_scene.exe`.

Linux: extract the archive and run:

```bash
./epoch_integrated_opengl_scene
```

The Linux binary targets Ubuntu 24.04-compatible x64 systems with X11, libpng, and an OpenGL 4.5-capable driver. Some archive tools may require restoring the executable bit with `chmod +x epoch_integrated_opengl_scene`.

## Platform status

| Platform | Context path | Status |
|---|---|---|
| Windows x64 | Win32 + WGL, OpenGL 4.5 core | Supported and built in CI with Visual Studio 2022 |
| Linux x64 | X11 + GLX, OpenGL 4.5 core | Supported and built in CI with GCC 14 and Ninja |
| Android | NativeActivity + EGL + OpenGL ES 3.x | Integration scaffold; renderer bridge is not complete |

Windows and Linux use the same `epoch.*` C++23 named-module graph. The reusable renderer target is `epoch_render_spine`.

## Renderer features

- forward metallic/roughness PBR
- normal, parallax, clearcoat, and transmission materials
- directional, point, and spotlight lighting
- projector-cookie spotlight
- directional and point-light shadow maps
- Tier 0 PCF and Tier 1 PCSS soft shadows
- HDR, bloom, tone mapping, gamma, fog, and FXAA
- render-to-texture camera feeds and planar mirror feed
- editable terrain with displacement and optional tessellation
- terrain-following billboard grass with a paintable placement mask
- cloth simulation with shadow casting
- water with scene-color refraction
- multiple particle emitters
- selectable objects backed by one instanced crate draw
- vertex-colored tree meshes
- persistent editor defaults and imported materials/models

## Tier policy

The tier split is a performance and compatibility budget, not an OpenGL rule.

### Tier 0 — mobile-oriented baseline

- one evaluated projector spotlight
- four finite-radius point lights
- one directional shadow map with bounded `3×3` PCF
- one point-light shadow cubemap
- forward PBR
- one billboard plane per grass instance
- ordinary hardware instancing
- vertex-displaced terrain without tessellation stages
- CPU cloth and particles
- water refraction, HDR, bloom, tone mapping, and fog
- no indirect draw, GPU queries, SSAO, tessellation, or PCSS

### Tier 1 — desktop additions

- all three authored spotlights
- blocker-search PCSS directional shadows
- crossed billboard planes
- hardware terrain tessellation
- indirect indexed submission
- SSAO and GPU timing queries

## Editor

The scene editor provides:

- viewport selection and group selection
- position, rotation, size, and relative-scale editing
- finite `− / value / +` adjustment controls
- click-to-type numeric fields and mouse scrubbing
- Enabled and Delete state
- editable materials and texture slots
- selectable terrain, water, cloth, lights, particle emitters, and instanced props
- paint/erase grass placement mask
- Save default, Reload, and Reset

Factory scene data is mirrored between typed hardcoded tables and `assets/editor/default_scene.cfg`.

Current factory invariants:

- 269 contiguous records
- no deleted tombstones
- UV scale `1.0` on every factory material
- 24 exact authored OBJ models reconstructed and checksum-validated at configure time

Run the validator directly with:

```bash
python3 tools/validate_scene_defaults.py
```

## Controls

| Input | Action |
|---|---|
| `W A S D` | Move camera |
| `Q / E` | Move down/up |
| Right mouse drag | Camera look |
| Left click | Select object or paint grass mask |
| `Ctrl` + left click | Toggle group selection |
| `Shift` + left click | Add group selection |
| Mouse wheel | Scroll the GUI panel |
| `F1` | Toggle Tier 0 / Tier 1 |
| `F2` | Toggle animated red/blue spotlights |
| `F3` | Toggle day/night cycle |
| `F4` | Reset scene |
| `Delete` | Delete selected editor object |
| `Escape` | Close application |

Numeric controls support:

- click a number to type it
- Backspace removes one character
- Enter commits
- Escape cancels
- drag horizontally to scrub
- Shift gives fine adjustment
- Ctrl gives coarse adjustment

## Build from source

### Windows

Requirements:

- Visual Studio 2022 with Desktop development with C++
- CMake 3.28 or newer
- Python 3

PowerShell:

```powershell
.\build_msvc.ps1
```

Or configure and build manually:

```powershell
cmake --preset msvc-vs2022
cmake --build --preset vs2022-release
```

The executable is written to `build\vs2022\bin\Release`.

### Linux

Requirements:

- CMake 3.28 or newer
- Ninja
- GCC 14 or newer
- Python 3
- X11, libpng, and OpenGL development packages

Ubuntu 24.04 dependencies:

```bash
sudo apt install cmake ninja-build g++-14 libx11-dev libpng-dev libgl1-mesa-dev
```

Build:

```bash
chmod +x build_linux.sh
./build_linux.sh
```

The executable is written to `build/linux-gcc/bin/Release`.

## Exact model packaging

The authored OBJ set is stored as nine contiguous base64 archive parts under `assets/default_pack/model_bundle_4_6_7`. During CMake configuration, `tools/prepare_model_assets.py`:

1. verifies all nine parts are present and contiguous
2. decodes the archive
3. verifies SHA-256 `5026d93ee4514264da5948c7c5b663e0f2890de2f7e0cdc2bbfaf2da8ac5c4fd`
4. rejects unsafe or unexpected archive entries
5. reconstructs all 24 exact OBJ files into the build tree
6. copies those models into the runtime binary package

A missing or damaged model now fails validation and configuration. It is never silently replaced with a cube.

## Architecture

```text
application
  └─ import epoch.app
      └─ epoch_render_spine
          ├─ context spine
          │   ├─ Win32/WGL
          │   └─ X11/GLX
          ├─ scene and resource spines
          ├─ renderer techniques
          ├─ EpochGui overlay
          └─ OpenGL resource ownership
```

Key source areas:

| Area | Path |
|---|---|
| Module interfaces | `src/epoch/modules` |
| Platform contexts | `src/epoch/platform` |
| OpenGL resources | `src/epoch/render/gl` |
| Render techniques | `src/epoch/render/techniques` |
| Scene construction/defaults | `src/epoch/render/world` |
| GUI/editor | `src/epoch/gui` |
| Android scaffold | `integration/android` |

## CI and releases

Every pull request validates and builds both desktop paths:

- Windows Visual Studio build
- Linux GCC 14 named-module build
- exact model archive checksum and contents
- 269 factory scene records
- Linux Xvfb startup smoke test
- absence of diagnostic cube fallback behavior
- ready-to-run binary package construction

Validated pushes to `main` publish Windows x64, Linux x64, and source archives to the matching GitHub release.

## Current limitations

- Android is an integration scaffold, not a completed mobile renderer port.
- The Linux binary targets Ubuntu 24.04-compatible userspace rather than being a universal AppImage.
- macOS is not implemented.
- Tier 0 is a desktop simulation of the intended mobile feature budget.
- The renderer remains a demonstration scene rather than a complete production engine.

## License

See `LICENSE`.
