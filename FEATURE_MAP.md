# EpochGui integrated scene feature map

| Technique / scene use | Tier 0 | Tier 1 addition |
|---|---|---|
| Native desktop shell | Win32/WGL or X11/GLX OpenGL 4.5 | Same |
| PBR materials | Metallic/roughness, Blinn compatibility, normal, parallax, clearcoat, transmission, alpha cutout | Same |
| Directional shadows | One map, slope bias, polygon offset, 3×3 PCF | PCSS blocker search and variable-radius 24-tap Poisson filtering |
| Point shadows | One 512px cubemap, six ordinary depth passes, bounded disk PCF | Higher disk sample count |
| Spotlights | One projector-cookie spotlight | Projector plus red and blue rotating spotlights |
| Terrain | 33×33 vertex-displaced grid and splat materials | Hardware tessellation and optional PN-style curvature |
| Billboards | Visible instanced vertex-expanded foliage, single plane | Perpendicular second plane for crossed clumps |
| Props | Hardware instancing | Optional indirect indexed submission |
| Cloth | Fixed-step Verlet, dynamic normals, shadow casting | Same |
| Water | Pool/lake vertex displacement, scene-color refraction, Fresnel, absorption, foam | Same |
| Particles | Multiple fire, mote, and mist emitters | Same |
| Render-to-texture | Security, overhead, ceiling camera and mirror targets | Same |
| Post processing | HDR, bloom, FXAA, ACES, exposure, gamma, fog | SSAO and GPU query diagnostics |

## Tier-0 exclusions

Tier 0 intentionally disables geometry shaders, tessellation stages, indirect draw, SSAO, GPU timer queries, PCSS blocker search, and the two decorative camera-room spotlights. This is a deterministic performance profile rather than an OpenGL/OpenGL ES restriction.

## Authored scene coverage

- hardcoded V5 factory scene and matching editable cfg
- original and duplicated reflecting pools with independent effect descriptors
- projector-cookie spotlight and physical fixture
- red/blue rotating spotlight fixtures
- corrected four-bench campfire assembly
- cloth flags/awning, terrain foliage, instanced loading-bay props, water, particles, labels, cameras, and debug proxies

`src/epoch/render/techniques/technique_catalog.cpp` remains the implementation truth table. Planned engine contracts are not reported as completed renderer features.
