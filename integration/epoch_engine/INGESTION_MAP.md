# EpochEngine ingestion map

This package is shaped for surgical extraction into EpochEngine. It is not intended to become a second engine core.

| Package boundary | EpochEngine destination |
|---|---|
| `src/epoch/context/context_spine.*` | existing context/window/frame orchestration |
| `src/epoch/render/render_spine.*` | backend-neutral render sequencing or render-graph assembly |
| `src/epoch/render/resource_spine.*` | existing `render.device` typed resource model and backend hooks |
| `src/epoch/render/scene/*` | ECS/visibility-extracted render data and material/light descriptors |
| `src/epoch/render/world/world_scene.*` | example content construction only; replace with scene/ECS loading |
| `src/epoch/render/techniques/*` | OpenGL backend technique implementations |
| `src/epoch/render/gl/*` | OpenGL backend-private allocation, loading, and command helpers |
| `src/epoch/platform/win32/*` | WGL context provider; do not expose through common render modules |
| `src/epoch/platform/linux/*` | X11/GLX context provider; replace or retain as an OpenGL platform adapter |
| `integration/android/*` | EGL/OpenGL ES NativeActivity pathway; not a validated backend |
| `src/epoch/gui/gui_overlay.*` | OpenGL replay adapter consuming EpochGui layout/controller output |
| `vendor/EpochGui` | replace with the canonical in-tree `EpochGui` target during ingestion |

## Central spine rules

1. Preserve typed handles and do not leak `GLuint` into scene or ECS data.
2. Feed the render scene from Epoch visibility extraction; techniques must not query gameplay state directly.
3. Convert direct pass calls into the engine render graph once its scheduling/target ownership is ready.
4. Preserve resource-before-context destruction.
5. Preserve material feature masks. Optional effects require both a global policy toggle and per-material eligibility.
6. Preserve capability truth. A descriptor or planned module is not backend implementation proof.
7. Move file paths into the asset registry and pass resolved resource handles to technique code.
8. Replace standalone point-light arrays and hard-coded emitters with bounded extracted descriptors.
9. Keep VSync off by default for this scene; let the engine's presentation policy own final pacing.
10. Keep EpochGui portable. OpenGL GUI replay belongs in the renderer adapter, not the reusable GUI library.

## Compiled module contracts

The Windows standalone build compiles these C++23 module interfaces through CMake `FILE_SET CXX_MODULES`. Linux consumes matching compatibility headers generated from the same contracts:

- `epoch.core.math`
- `epoch.core.log`
- `epoch.core.time`
- `epoch.context.input`
- `epoch.context.frame`
- `epoch.context.spine`
- `epoch.render.types`
- `epoch.render.tier`
- `epoch.render.scene`
- `epoch.render.technique.context`
- `epoch.render.techniques.catalog`
- `epoch.render.spine`
- `epoch.app`

During EpochEngine ingestion, merge these contracts into the existing module graph rather than creating duplicate module names. Public engine-facing contracts must remain modules. Headers under `platform/win32`, `platform/linux`, `render/gl`, `render/techniques`, and `detail` are private backend implementation boundaries, not shared Epoch interfaces.

## Standalone-only decisions to replace

- hand-authored world placement
- WIC image decoding and the minimal OBJ importer as the production asset pipeline
- one directional shadow map for every view
- fixed bloom kernel and post chain
- standalone emitter positions
- direct OpenGL uniforms instead of engine binding sets/pipelines
