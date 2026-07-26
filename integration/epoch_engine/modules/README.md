# EpochEngine module contracts

These `.ixx` files are backend-neutral extraction contracts for merging into EpochEngine. They are intentionally excluded from this standalone MSVC target so they cannot collide with the engine's existing module graph.

- `context.spine.ixx` mirrors frame/input/configuration ownership. VSync is off by default.
- `render.spine.ixx` exposes typed handles and capability truth without OpenGL types.
- `render.object.ixx` mirrors selectively routed material features plus ordinary, water, and cloth scene objects.

The standalone implementation uses headers and `.cpp` files so it can build independently. During ingestion, preserve the contracts and move backend implementation behind EpochEngine's established renderer/device modules rather than importing this project as a second renderer core.
