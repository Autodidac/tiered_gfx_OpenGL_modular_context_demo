# EpochGui renderer-spine architecture

## Platform boundary

```text
Application
  -> ContextSpine
       -> Win32Window + WGLContext              Windows
       -> X11GlxWindow                          Linux
       -> AndroidEglShell pathway               Android scaffold
  -> RenderSpine
       -> SceneSpine                            backend-neutral scene data
       -> ResourceSpine                         typed opaque handles
       -> isolated render techniques
       -> EpochGui OpenGL replay adapter
```

The Windows and Linux shells expose the same frame/input/window contract. Scene, material, light, editor, and simulation definitions do not contain HWND, HDC, X11 Window, GLXContext, EGLSurface, or raw OpenGL ownership.

## Build-language boundary

- Windows/MSVC uses the C++23 module interface files under `src/epoch/modules`.
- Windows and Linux compile the same `epoch.*` C++23 named modules; compatibility headers remain only as a future constrained-toolchain fallback.
- Both compile the same implementation files and renderer behavior.

This isolates toolchain module maturity from renderer portability without maintaining two renderers.

## Lifetime guarantee

`Application` owns `ContextSpine` before `RenderSpine`. Members are destroyed in reverse order, so OpenGL resources are released while the native context still exists.

## Frame sequence

1. Pump native events and update frame timing.
2. Apply VSync, limiter, Tier-0/Tier-1 policy, editor state, and camera input.
3. Update authored animation, cloth, particles, water parameters, and light directions.
4. Resize HDR, bloom, depth, and render-to-texture targets when required.
5. Render independent camera/mirror targets.
6. Render static geometry and cloth into the directional shadow map.
7. Render the selected point-light shadow cubemap.
8. Bind HDR MRT and render sky/environment.
9. Render terrain using vertex displacement or Tier-1 tessellation.
10. Render opaque PBR objects and sorted transmission objects.
11. Render visible billboard foliage, instanced props, RTT surfaces, cloth, particles, and water.
12. Run bloom and final post processing.
13. Replay EpochGui and swap through WGL or GLX.

## Tiered shadows

The directional shadow texture and caster pass are shared by both tiers.

- Tier 0 performs nine percentage-closer comparisons in a fixed 3×3 kernel. Its cost is bounded and the result is softly filtered without blocker search.
- Tier 1 performs a 16-sample blocker search, estimates receiver/blocker separation, and filters with a 24-sample Poisson kernel whose radius expands with the estimated penumbra.
- Point-light shadows use one cubemap and a bounded disk-PCF sample count.

## Billboards

Foliage uses one instanced quad expanded and bent in the vertex shader. Tier 0 submits one camera-facing plane; Tier 1 submits a perpendicular second pass to create crossed clumps. Alpha-test threshold, authored blade dimensions, density floor, and scene exclusions are owned by `BillboardFoliageTechnique`.

## Mobile pathway boundary

The Android scaffold owns only NativeActivity lifecycle and EGL. A complete mobile backend should:

- use OpenGL ES shader variants and `AAssetManager`
- preserve the SceneSpine/ResourceSpine contracts
- compile Tier-0-only techniques
- omit tessellation, PCSS, indirect draw, SSAO, and desktop query paths
- translate touch/gamepad input into `context::InputState`

No Android runtime claim is made by this source package.
