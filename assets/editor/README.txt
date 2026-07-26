Epoch scene editor import and save folder

Authored/default behavior:
- The initial scene is constructed from the typed hard-coded authored table.
- default_scene.cfg is then loaded automatically as the persistent user override.
- Reset restores the hard-coded authored values, never values inferred from the cfg.
- Reload restores the hard-coded baseline and reapplies default_scene.cfg.
- Save default atomically writes the current editor state as EPOCH_SCENE_EDITOR_V4.
- If the cfg is missing, it is regenerated from the same hard-coded authored table.

Selection and debug:
- Left-click selects an object or authored group.
- Ctrl-click or Shift-click adds/removes entities from a manual multi-selection.
- F7 or System > Debug / x-ray view exposes hidden objects and effect proxies.
- Debug colors identify water, cloth, RTT displays, labels, point lights,
  spotlights, foliage, particles, and hidden ordinary meshes.
- Alt-click prioritizes effect handles.
- Hide is reversible; select a hidden object in debug view and choose Restore.

Inspector:
- Position, Size, and Scale are independent persistent values.
- Click any numeric field to type an exact value; Enter or clicking elsewhere commits it.
- Drag the slider beside the field for continuous mouse control.
- Mouse wheel adjusts a hovered numeric field; Shift is 10x and Ctrl is 0.1x.
- Size changes absolute authored dimensions.
- Scale is a separate relative multiplier; render size is Size * Scale.
- Duplicate copies safe regular objects and handmade groups.

Model import:
- Load OBJ... copies the selected model to assets/editor/imports/models.
- Imported models are discovered at startup.

Texture/material editing:
- Apply selects an existing base material.
- Load texture imports Albedo, Normal, or ORM into a unique editable material.
- Reset slot restores the chosen texture slot from its base material.
- UV scale, metallic, roughness, and normal strength are editable.
- Imported texture paths and material controls persist in V4 cfg files and are
  rebuilt through ResourceSpine on the next launch.
