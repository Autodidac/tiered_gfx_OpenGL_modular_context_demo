#pragma once


#include <filesystem>
#include "epoch/render/gl/shader_program.hpp"
#include "epoch/render/resource_spine.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.scene.hpp"
#else
import epoch.render.scene;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.technique.context.hpp"
#else
import epoch.render.technique.context;
#endif

namespace epoch::render::techniques {

class InstancedPropsTechnique {
public:
    InstancedPropsTechnique(const std::filesystem::path& shader_root, ResourceSpine& resources, MeshHandle mesh);
    InstancedPropsTechnique(const InstancedPropsTechnique&) = delete;
    InstancedPropsTechnique& operator=(const InstancedPropsTechnique&) = delete;
    ~InstancedPropsTechnique();
    void render(const TechniqueContext& frame, const scene::SceneSpine& scene, const ResourceSpine& resources) const;

private:
    struct DrawElementsIndirectCommand {
        GLuint count{};
        GLuint instance_count{};
        GLuint first_index{};
        GLuint base_vertex{};
        GLuint base_instance{};
    };

    gl::ShaderProgram shader_;
    MeshHandle mesh_{};
    TextureHandle albedo_{};
    TextureHandle orm_{};
    GLuint indirect_buffer_{};
    GLuint index_count_{};
};

} // namespace epoch::render::techniques
