# Authored scene defaults

The source-owned `hardcoded_scene_defaults.inc` and
`hardcoded_material_defaults.inc` tables are authoritative. Together they contain
all 246 factory scene records, material selections, material scalar controls, water
layout/tuning, corrected benches, rotating red/blue camera-room spotlights, and
debug/effect proxies.

Factory behavior:

1. `WorldSceneBuilder` constructs the complete scene.
2. The hard-coded transform/effect table applies all 246 records.
3. The hard-coded material table applies the approved non-bench texture/material state.
4. `default_scene.cfg` loads automatically as the persistent user override.
5. **Reset** restores the compiled factory state.
6. **Reload** restores the compiled state and reapplies the cfg.
7. **Save default** atomically writes V4 state.

No factory record is deleted or hidden. Preset visibility remains normal runtime
filtering and is not an editor deletion.

The uploaded scene edits were merged selectively: water geometry/tuning and
non-bench material state were retained. The corrected east-bench-derived geometry,
new spotlight rig, transmission crystal, and complete debug proxy set remain from
4.5.31.
