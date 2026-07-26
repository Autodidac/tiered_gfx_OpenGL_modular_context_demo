module;
#include <array>
#include <span>

module epoch.render.techniques.catalog;

namespace epoch::render::techniques {
namespace {
constexpr auto catalog = std::to_array<TechniqueCatalogEntry>({
    {"platform.desktop.gl45", "Native Windows and Linux OpenGL 4.5 context", TechniqueGroup::foundation, TechniqueStatus::implemented, "context.spine", "Win32/WGL and X11/GLX shells share explicit GL loading, input, resize, VSync and frame pacing"},
    {"resource.explicit", "Explicit renderer resources", TechniqueGroup::foundation, TechniqueStatus::implemented, "resource.spine", "typed mesh, texture, cubemap and material handles with backend-private GL objects"},
    {"target.hdr", "HDR multiple-render-target frame", TechniqueGroup::foundation, TechniqueStatus::implemented, "frame.targets", "floating-point scene, brightness and sampleable depth attachments"},
    {"target.rtt", "Render-to-texture", TechniqueGroup::foundation, TechniqueStatus::implemented, "render.to.texture", "security, overhead-site and planar-mirror feeds use ordinary color/depth framebuffer render targets"},

    {"texture.srgb", "sRGB upload and linear-light shading", TechniqueGroup::materials, TechniqueStatus::implemented, "resource.spine", "color maps use sRGB while data maps remain linear"},
    {"texture.mip.aniso", "Mipmaps and anisotropic filtering", TechniqueGroup::materials, TechniqueStatus::implemented, "resource.spine", "terrain, masonry, timber and props use capability-clamped anisotropy"},
    {"material.pbr", "Metallic/roughness PBR", TechniqueGroup::materials, TechniqueStatus::implemented, "pbr.forward", "metal, ceramic, timber, stone, fabric and foliage reuse one routed material path"},
    {"material.blinn", "Blinn-Phong compatibility shading", TechniqueGroup::materials, TechniqueStatus::implemented, "pbr.forward", "GUI-selectable compatibility view over the same scene"},
    {"material.normal", "Tangent-space normal mapping", TechniqueGroup::materials, TechniqueStatus::implemented, "pbr.forward", "handed tangents and inverse-transpose normal transforms across architecture and props"},
    {"material.parallax", "Parallax occlusion mapping", TechniqueGroup::materials, TechniqueStatus::implemented, "pbr.forward", "the masonry room uses a planar height-mapped brick receiver with separate structural caps"},
    {"material.clearcoat", "Layered clearcoat", TechniqueGroup::materials, TechniqueStatus::implemented, "pbr.forward", "brushed aluminum, automotive paint and carbon fiber are isolated in dedicated rooms"},
    {"material.transmission", "Environment transmission/refraction", TechniqueGroup::materials, TechniqueStatus::implemented, "pbr.forward", "the transmission room and lamp globes use frosted glass environment refraction"},
    {"material.alpha", "Alpha-tested material", TechniqueGroup::materials, TechniqueStatus::implemented, "pbr.forward", "foliage material and billboard vegetation"},

    {"light.directional", "Directional light", TechniqueGroup::lighting, TechniqueStatus::implemented, "pbr.forward", "time-of-day presets alter one persistent scene"},
    {"light.point", "Multiple finite-radius point lights", TechniqueGroup::lighting, TechniqueStatus::implemented, "pbr.forward", "three room pendants and the nearby campfire keep local lighting visible"},
    {"light.spot", "Spot light", TechniqueGroup::lighting, TechniqueStatus::implemented, "pbr.forward", "a close projector spotlight targets the parallax brick room"},
    {"light.projector", "Projected texture spotlight", TechniqueGroup::lighting, TechniqueStatus::implemented, "pbr.forward", "the parallax brick receiver opts into the projected cookie path"},
    {"shadow.directional.soft", "Tiered directional soft shadows", TechniqueGroup::lighting, TechniqueStatus::implemented, "shadow.mapping", "Tier 0 uses bounded 3x3 PCF; Tier 1 uses blocker search and variable-penumbra PCSS filtering"},
    {"environment.sky", "Cubemap sky and environment response", TechniqueGroup::lighting, TechniqueStatus::implemented, "sky.environment", "sky, metal, glass, clearcoat, cloth and water share the environment"},

    {"geometry.billboard", "Instanced vertex-expanded billboards", TechniqueGroup::geometry, TechniqueStatus::implemented, "billboard.foliage", "Tier-0 foliage expands camera-facing quads in the vertex shader; no geometry shader is required"},
    {"geometry.instancing", "Hardware instancing", TechniqueGroup::geometry, TechniqueStatus::implemented, "instanced.props", "the labeled loading bay draws twelve crates from one mesh/material batch"},
    {"geometry.indirect", "Indirect indexed drawing", TechniqueGroup::geometry, TechniqueStatus::implemented, "instanced.props", "ordinary instancing is Tier 0; indirect submission is a configurable Tier-1 path"},
    {"geometry.tessellation", "Tessellated height-map terrain", TechniqueGroup::geometry, TechniqueStatus::implemented, "tessellation", "Tier 0 uses a vertex-displaced 33x33 grid; Tier 1 uses distance-adaptive hardware tessellation"},
    {"geometry.pn", "PN-style curved tessellation", TechniqueGroup::geometry, TechniqueStatus::implemented, "tessellation", "optional Tier-1 patch curvature; the Tier-0 path never requires tessellation"},

    {"simulation.object", "Transform animation", TechniqueGroup::simulation, TechniqueStatus::implemented, "scene.spine", "room display pieces rotate slowly while architecture remains stable"},
    {"simulation.cloth", "Fixed-step Verlet cloth", TechniqueGroup::simulation, TechniqueStatus::implemented, "cloth.simulation", "a 48x26 American flag and woven entrance awning use separate pin modes and cast shadows"},
    {"simulation.water", "Screen-refractive water", TechniqueGroup::simulation, TechniqueStatus::implemented, "water.simulation", "a bordered reflecting pool and distant lake use separate low-slope profiles, scene-color refraction and Fresnel response"},
    {"simulation.particles", "Multi-emitter CPU particles", TechniqueGroup::simulation, TechniqueStatus::implemented, "particle.system", "campfire sparks and reflecting-pool motes/mist use distinct blend behavior"},

    {"post.hdr", "HDR and ACES tone mapping", TechniqueGroup::post_process, TechniqueStatus::implemented, "post.process", "one tone-mapped final frame with exposure control"},
    {"post.bloom", "Separable Gaussian bloom", TechniqueGroup::post_process, TechniqueStatus::implemented, "bloom", "lamp bulbs, fire and emissive in-world displays contribute"},
    {"post.fxaa", "FXAA", TechniqueGroup::post_process, TechniqueStatus::implemented, "post.process", "configurable final-frame antialiasing after 4x MSAA scene rasterization"},
    {"post.ssao", "View-reconstructed screen-space AO", TechniqueGroup::post_process, TechniqueStatus::implemented, "post.process", "optional Tier-1 view-space contact AO with radius and strength controls; disabled by Tier 0"},
    {"post.fog", "Distance fog", TechniqueGroup::post_process, TechniqueStatus::implemented, "pbr.forward", "PBR, terrain, cloth and water share the same fog controls"},
    {"post.toon", "Material-routed toon and rim lighting", TechniqueGroup::post_process, TechniqueStatus::implemented, "pbr.forward", "only explicitly routed materials can opt in; no global toon pass is applied"},

    {"debug.views", "Material and shadow debug views", TechniqueGroup::diagnostics, TechniqueStatus::implemented, "pbr.forward", "normal, UV, roughness, metallic, AO and shadow visibility views"},
    {"debug.gl", "OpenGL debug output", TechniqueGroup::diagnostics, TechniqueStatus::implemented, "render.spine", "synchronous driver messages in debug builds when supported"},
    {"debug.gpuquery", "GPU timer queries", TechniqueGroup::diagnostics, TechniqueStatus::implemented, "gpu.profiler", "optional Tier-1 backend timing query path; disabled by the Tier-0 profile"},

    {"shadow.point", "Point-light cubemap shadows", TechniqueGroup::lighting, TechniqueStatus::implemented, "point.shadow.mapping", "one Tier-0 512px cubemap uses six standard framebuffer passes with no geometry shader or vendor extension"},
    {"shadow.cascade", "Cascaded directional shadows", TechniqueGroup::lighting, TechniqueStatus::planned_engine_contract, "shadow family", "planned after camera-stable split ownership is in the render graph"},
    {"render.deferred", "Deferred G-buffer lighting", TechniqueGroup::post_process, TechniqueStatus::planned_engine_contract, "render graph", "forward PBR is the implemented scene path"},
    {"animation.skinning", "GPU skeletal animation", TechniqueGroup::simulation, TechniqueStatus::planned_engine_contract, "animation spine", "the supplied humanoids are static meshes"},
    {"particles.compute", "Compute/SSBO particles", TechniqueGroup::simulation, TechniqueStatus::planned_engine_contract, "simulation spine", "CPU particles remain the implemented baseline"},
    {"selection.outline", "Picking and selected-object outline", TechniqueGroup::diagnostics, TechniqueStatus::planned_engine_contract, "editor selection", "belongs behind Epoch editor selection ownership"},
    {"bindless.texture", "Bindless texture residency", TechniqueGroup::foundation, TechniqueStatus::capability_gated, "resource spine", "extension-gated and intentionally outside the GTX 1660 compatibility path"}
});
}

std::span<const TechniqueCatalogEntry> technique_catalog() noexcept { return catalog; }

std::size_t implemented_technique_count() noexcept {
    std::size_t result{};
    for (const auto& entry : catalog)
        if (entry.status == TechniqueStatus::implemented) ++result;
    return result;
}

} // namespace epoch::render::techniques
