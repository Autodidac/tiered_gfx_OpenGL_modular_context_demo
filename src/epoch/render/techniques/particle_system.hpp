#pragma once



#include <array>
#include <filesystem>
#include "epoch/render/gl/shader_program.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.core.math.hpp"
#else
import epoch.core.math;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.technique.context.hpp"
#else
import epoch.render.technique.context;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.scene.hpp"
#else
import epoch.render.scene;
#endif

namespace epoch::render::techniques {
class ParticleSystemTechnique {
public:
    explicit ParticleSystemTechnique(const std::filesystem::path& shader_root);
    ParticleSystemTechnique(const ParticleSystemTechnique&)=delete;
    ParticleSystemTechnique& operator=(const ParticleSystemTechnique&)=delete;
    ~ParticleSystemTechnique();
    void update(float delta_seconds,float elapsed_seconds,const context::RuntimeControls& controls);
    void render(const TechniqueContext& frame, const scene::SceneSpine& scene) const;
private:
    static constexpr std::size_t fire_count=192;
    static constexpr std::size_t mote_count=96;
    static constexpr std::size_t mist_count=96;
    static constexpr std::size_t particle_count=fire_count+mote_count+mist_count;
    struct Particle { math::Vec3 position{}; math::Vec3 velocity{}; math::Vec4 color{}; float size{}; float age{}; float lifetime{}; };
    struct GpuParticle { math::Vec3 position{}; math::Vec4 color{}; float size{}; };
    void respawn(Particle& particle,std::size_t index,float elapsed_seconds);
    gl::ShaderProgram shader_;
    GLuint vao_{};GLuint vbo_{};
    std::array<Particle,particle_count> particles_{};
    mutable std::array<GpuParticle,particle_count> gpu_{};
};
}
