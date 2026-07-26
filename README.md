# EpochGui Integrated OpenGL Scene 4.6.6

A C++23 renderer/editor demonstration with one persistent authored scene, metallic/roughness PBR, water, cloth, particles, billboards, render-to-texture, tiered shadows, and hardcoded factory defaults.

## 4.6.6 terrain, grass placement, tree defaults, and sun-disc correction

- Terrain is a first-class selectable editor object with persistent position, size, scale, Enabled/Delete state, and height-scale controls.
- Grass samples the edited terrain transform and height field, including valleys, instead of using a flat placement plane.
- Grass placement uses a persistent 64x64 paintable mask; automatic shrunken solid-object footprints remain excluded after painting.
- The five perimeter-tree transforms match `default_scene(15).cfg` exactly.
- The animated sun is additive over the environment and no longer cuts a dark corona into the sky cubemap.


## 4.6.5 sun and factory-default corrections

- Promoted `default_scene(15).cfg` to the compiled factory baseline.
- Direct solar radiance now fades to zero below the horizon instead of acting as an underground night light.
- Directional and cloth shadow passes stop below the horizon after clearing the shadow target.
- The light-view up vector switches axes at steep solar angles, preventing unstable shadow matrices.
- The deleted planar-mirror camera housing is physically absent from the factory scene rather than retained as a tombstone.

## 4.6.4 editor/model corrections

- Corrected the oak and pine source meshes: only the brown trunk cylinder is rotated from the authored Z axis to the vertical Y axis.
- The five retained perimeter trees are grounded at terrain height and use canopy-centered selection bounds.
- All 12 crate instances are ordinary selectable editor entries while remaining one instanced renderer batch. Position, rotation, scale, Enabled, and Delete update the batched draw directly.

## Platform status

| Platform | Window/context path | Status in this package |
|---|---|---|
| Windows x64 | Win32 + WGL, OpenGL 4.5 core | Native build path retained through Visual Studio 2022/2026 presets |
| Linux x64 | X11 + GLX, OpenGL 4.5 core | Configured, compiled, linked, and run under Xvfb with GCC 14 named modules |
| Android | NativeActivity + EGL + OpenGL ES 3.x | Integration pathway only; no SDK/NDK validation claimed |

The reusable renderer target is `epoch_render_spine`. Windows uses the C++23 module files. Windows and Linux use the same C++23 named-module graph. The verified Linux preset requires Ninja and GCC 14+. Clang also needs a matching `clang-scan-deps` executable and is not the packaged default preset.

## Linux build

Dependencies: CMake 3.28+, Ninja, a C++23 compiler, X11 development files, libpng development files, and a working OpenGL/GLX driver.

```bash
chmod +x build_linux.sh
./build_linux.sh linux-gcc-release
```

Clang is also supported:

```bash
./build_linux.sh linux-clang-release
```

Output:

```text
build/linux-gcc/bin/Release/epoch_integrated_opengl_scene
```

Run from a graphical X11/XWayland session:

```bash
./build/linux-gcc/bin/Release/epoch_integrated_opengl_scene
```

## Windows build

Open the project directory and run:

```text
open_msvc.bat
```

or:

```text
build_msvc.bat Release
```

The Windows executable retains the application icon, Win32 input, WGL context creation, VSync control, and C++23 module build.

## Android pathway

`integration/android/` contains a NativeActivity manifest, NDK CMake target, EGL lifecycle, OpenGL ES proof frame, and explicit Tier-0 budget. It is deliberately not presented as a completed Android port. The remaining bridge is asset loading through `AAssetManager`, GLES shader variants, and connection of the existing scene/render ownership to the NativeActivity loop.

## Tier policy

The single-spotlight limit is a **Tier-0 performance budget**, not an API restriction.

### Tier 0 — mobile-oriented baseline

- one evaluated projector spotlight
- four finite-radius point lights
- one directional shadow map with bounded 3×3 PCF
- one point-light shadow cubemap
- forward metallic/roughness PBR
- normal mapping and optional parallax
- vertex-expanded instanced billboards, one crossed plane pass
- ordinary hardware instancing
- vertex-displaced terrain; no tessellation stage
- CPU cloth and particles
- vertex water with scene-color refraction
- HDR, bloom, FXAA, tone mapping, exposure, gamma, and fog
- no geometry shaders, indirect draw, SSAO, GPU queries, or PCSS

### Tier 1 — desktop additions

- all three authored spotlights
- blocker-search PCSS directional shadows with variable penumbra and 24 Poisson filter taps
- crossed two-plane billboard pass
- hardware tessellation and optional PN-style curvature
- indirect indexed drawing
- SSAO and GPU timing queries

## Foliage and tree correction

The grass renderer retains its explicit visible instance floor, corrected alpha cutoff, and optional crossed Tier-1 pass. `default_scene(14).cfg` now supplies the compiled grass-region position, bounds, scale, density, blade scale, and sway defaults.

The eight perimeter-tree OBJs are solid low-poly meshes with authored vertex colors and no UV coordinates. They previously used an unrelated alpha-cutout foliage texture, so the main PBR pass discarded them while the shadow pass still rendered their geometry. OBJ vertex colors are now preserved as a mesh attribute, multiplied into PBR albedo, and the trees use an opaque vertex-colored material. Their visible geometry and their existing shadows therefore match.

## Scene defaults and editor

The approved scene state is compiled into typed factory tables and mirrored to `assets/editor/default_scene.cfg`.

- 260 contiguous factory records
- no deleted factory records
- UV scale `1.0` on all compiled material records
- original and duplicated reflecting pools, each with independent water, mote, and mist emitters
- restored projector-cookie spotlight plus red/blue camera-room lights
- `EpochGui` branding
- cloth wind default `0.1`

The inspector retains `− / editable number / +`. Clicking enters exact text; Backspace removes one character; dragging the number scrubs it with a horizontal-resize cursor. **Enabled** controls visibility/effect activity, while **Delete** removes an item from persistent editor state.

## Runtime controls

- `WASD`: move
- `Q/E`: move down/up
- `Shift`: accelerate
- hold right mouse: camera look
- `F1`: wireframe
- `F2`: VSync
- `F3`: EpochGui panel
- `F4`: bloom
- `F5`: shadows
- `F6`: animation/simulation clock
- `F7`: editor debug/x-ray view
- `F10`: help strip
- `Esc`: exit

See `ARCHITECTURE.md`, `FEATURE_MAP.md`, and `integration/epoch_engine/INGESTION_MAP.md` for ownership and engine-ingestion details.
