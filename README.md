# Tiered OpenGL Modular Context Demo

A C++23 named-module renderer and scene editor demonstrating a practical **Tier 0 / Tier 1 graphics architecture** on one authored OpenGL scene.

The project is intended as a compact reference for cross-platform context creation, explicit renderer ownership, metallic/roughness PBR, editable scene data, and capability-budgeted rendering paths.

## Platform status

| Platform | Context path | Status |
|---|---|---|
| Windows x64 | Win32 + WGL, OpenGL 4.5 core | Supported with Visual Studio 2022 or newer |
| Linux x64 | X11 + GLX, OpenGL 4.5 core | Supported with GCC 14+, Ninja, X11 and libpng |
| Android | NativeActivity + EGL + OpenGL ES 3.x | Integration scaffold only; renderer bridge is not complete |

Windows and Linux use the same `epoch.*` C++23 named-module graph. The reusable renderer target is `epoch_render_spine`.

## Renderer features

- forward metallic/roughness PBR
- normal, parallax, clearcoat and transmission materials
- directional, point and spotlight lighting
- projector-cookie spotlight
- directional and point-light shadow maps
- Tier 0 PCF and Tier 1 PCSS soft shadows
- HDR, bloom, tone mapping, gamma, fog and FXAA
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

The tier split is a **performance and compatibility budget**, not an OpenGL rule.

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
- water refraction, HDR, bloom, tone mapping and fog
- no indirect draw, GPU queries, SSAO, tessellation or PCSS

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
- position, rotation, size and relative scale editing
- finite `− / value / +` adjustment controls
- click-to-type numeric fields and mouse scrubbing
- Enabled and Delete state
- editable materials and texture slots
- selectable terrain, water, cloth, lights, particle emitters and instanced props
- paint/erase grass placement mask
- Save default, Reload and Reset

Factory scene data is mirrored between typed hardcoded tables and `assets/editor/default_scene.cfg`.

Current factory state:

- **269 contiguous records**
- no deleted tombstones
- UV scale `1.0` on every compiled material record
- two independent reflecting pools and emitter sets
- five editable perimeter trees
- twelve selectable crate instances rendered as one batch

The grass paint mask is stored at `assets/editor/grass_placement_mask.pgm`.

## Build on Linux

Requirements:

- CMake 3.28+
- Ninja 1.11+
- GCC 14+
- X11 development files
- libpng development files
- OpenGL/GLX development files and driver

On Debian/Ubuntu-like systems:

```bash
sudo apt install g++-14 ninja-build libx11-dev libpng-dev libgl1-mesa-dev
./build_linux.sh
```

The script prefers `g++-14`, then accepts `g++` only when it reports GCC 14 or newer.

Output:

```text
build/linux-gcc/bin/Release/epoch_integrated_opengl_scene
```

Run it from an X11 or XWayland graphical session:

```bash
./build/linux-gcc/bin/Release/epoch_integrated_opengl_scene
```

## Build on Windows

Install Visual Studio 2022 or newer with **Desktop development with C++**, then run:

```text
build_msvc.bat Release
```

To generate and open the Visual Studio solution:

```text
open_msvc.bat
```

The scripts detect the newest supported installed Visual Studio generator.

## Controls

| Input | Action |
|---|---|
| `WASD` | Move camera |
| `Q / E` | Move down / up |
| `Shift` | Accelerate movement |
| Right mouse | Camera look |
| `F1` | Wireframe |
| `F2` | VSync |
| `F3` | EpochGui panel |
| `F4` | Bloom |
| `F5` | Shadows |
| `F6` | Animation/simulation clock |
| `F7` | Editor debug/x-ray view |
| `F10` | Help strip |
| `Esc` | Exit |

## Validation

Validate the hardcoded scene tables against the editable cfg:

```bash
python3 tools/validate_scene_defaults.py
```

The validator checks sequential indices, matching names/counts, deleted records and the UV-scale invariant.

## Project layout

```text
src/epoch/modules/       C++23 module interfaces
src/epoch/context/       frame timing, input and platform-facing context spine
src/epoch/platform/      Win32/WGL and X11/GLX implementations
src/epoch/render/        resources, scene, renderer passes and world construction
src/epoch/gui/           EpochGui editor overlay
assets/shaders/          GLSL programs
assets/editor/           editable cfg and grass placement mask
integration/android/     Android NativeActivity/EGL pathway
integration/epoch_engine ingestion notes for EpochEngine
vendor/EpochGui/         GUI library
```

See `ARCHITECTURE.md` and `FEATURE_MAP.md` for ownership boundaries and the implemented technique map.

## Known limitations

- Android contains a platform pathway, not a completed application port.
- Linux model/texture import does not yet provide a native file-picker dialog.
- The desktop renderer requires an OpenGL 4.5 core context.

## License

See `LICENSE`.
