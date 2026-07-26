#include "epoch/render/techniques/particle_system.hpp"
#include "epoch/render/gl/gl_api.hpp"
#include <cmath>
#include <cstddef>

namespace epoch::render::techniques {
namespace {
float hash(std::size_t value){value^=value>>16;value*=0x7feb352dU;value^=value>>15;value*=0x846ca68bU;value^=value>>16;return static_cast<float>(value&0xffffU)/65535.0f;}
}

ParticleSystemTechnique::ParticleSystemTechnique(const std::filesystem::path& root)
    :shader_{root/"particles/particles.vert",root/"particles/particles.frag"}{
    gl::GenVertexArrays(1,&vao_);gl::GenBuffers(1,&vbo_);gl::BindVertexArray(vao_);gl::BindBuffer(gl::array_buffer,vbo_);constexpr GLsizei stride=sizeof(GpuParticle);
    gl::EnableVertexAttribArray(0);gl::VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<void*>(offsetof(GpuParticle,position)));
    gl::EnableVertexAttribArray(1);gl::VertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<void*>(offsetof(GpuParticle,color)));
    gl::EnableVertexAttribArray(2);gl::VertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<void*>(offsetof(GpuParticle,size)));
    for(std::size_t i=0;i<particles_.size();++i){respawn(particles_[i],i,0.0f);particles_[i].age=hash(i+91)*particles_[i].lifetime;}
}
ParticleSystemTechnique::~ParticleSystemTechnique(){if(vbo_)gl::DeleteBuffers(1,&vbo_);if(vao_)gl::DeleteVertexArrays(1,&vao_);}

void ParticleSystemTechnique::respawn(Particle& p,std::size_t i,float elapsed){
    const float a=hash(i*13+static_cast<std::size_t>(elapsed*17.0f))*6.2831853f;
    if(i<fire_count){
        const float radius=.05f+hash(i*31)*.48f;
        p.position={std::cos(a)*radius,0.08f+hash(i*5)*0.12f,std::sin(a)*radius};
        p.velocity={std::cos(a)*(.05f+hash(i*7)*.15f),.75f+hash(i*11)*1.55f,std::sin(a)*(.05f+hash(i*19)*.15f)};
        const float warm=hash(i*23);p.color={1.0f,.18f+warm*.48f,.025f,.72f};
        p.size=30.0f+hash(i*29)*38.0f;p.lifetime=.8f+hash(i*37)*1.5f;
    }else if(i<fire_count+mote_count){
        const std::size_t local=i-fire_count;
        const float radius=0.6f+hash(local*17)*2.2f;
        p.position={std::cos(a)*radius,.35f+hash(local*7)*1.8f,std::sin(a)*radius};
        p.velocity={-std::sin(a)*(.10f+hash(local*3)*.18f),(.05f+hash(local*11)*.12f),std::cos(a)*(.10f+hash(local*5)*.18f)};
        p.color={.10f+.18f*hash(local),.55f+.35f*hash(local*19),1.0f,.62f};
        p.size=18.0f+hash(local*29)*20.0f;p.lifetime=3.5f+hash(local*37)*4.5f;
    }else{
        const std::size_t local=i-fire_count-mote_count;
        p.position={(hash(local*11)-.5f)*4.2f,.05f+hash(local*13)*.35f,(hash(local*17)-.5f)*1.6f};
        p.velocity={(hash(local*3)-.5f)*.12f,.08f+hash(local*5)*.13f,(hash(local*7)-.5f)*.05f};
        p.color={.48f,.72f,.80f,.12f};p.size=68.0f+hash(local*29)*72.0f;p.lifetime=3.0f+hash(local*37)*3.5f;
    }
    p.age=0.0f;
}

void ParticleSystemTechnique::update(float dt,float elapsed,const context::RuntimeControls& controls){
    if(!controls.particles)return;dt*=controls.animation_speed;
    for(std::size_t i=0;i<particles_.size();++i){
        auto& p=particles_[i];p.age+=dt;if(p.age>=p.lifetime){respawn(p,i,elapsed);continue;}
        if(i<fire_count){p.velocity.y-=.36f*dt;p.position+=p.velocity*dt;}
        else if(i<fire_count+mote_count){p.velocity.x+=std::sin(elapsed*1.3f+float(i))*.018f*dt;p.velocity.z+=std::cos(elapsed*.9f+float(i))*.018f*dt;p.position+=p.velocity*dt;}
        else{p.position+=p.velocity*dt;}
    }
}

void ParticleSystemTechnique::render(const TechniqueContext& frame, const scene::SceneSpine& scene) const {
    if(!frame.controls.particles)return;

    gl::Enable(gl::program_point_size);
    gl::DepthMask(GL_FALSE);
    shader_.bind();
    shader_.set("uViewProjection",frame.view_projection);
    gl::BindVertexArray(vao_);
    gl::BindBuffer(gl::array_buffer,vbo_);
    gl::BufferData(gl::array_buffer,static_cast<gl::GLsizeiptr>(sizeof(gpu_)),nullptr,gl::dynamic_draw);

    const auto upload_and_draw=[&](std::size_t first,std::size_t count,math::Vec3 origin,float size_scale,float intensity,bool additive){
        if(intensity<=0.0f || count==0u) return;
        for(std::size_t i=0;i<count;++i){
            const auto& p=particles_[first+i];
            const float life=1.0f-p.age/p.lifetime;
            const bool fire=first==0u;
            const bool mote=first==fire_count;
            const float alpha=(fire?life:(mote?(0.35f+0.65f*std::sin(life*3.14159265f)):std::sin(life*3.14159265f)))
                * intensity;
            gpu_[first+i]={origin+p.position*size_scale,{p.color.x,p.color.y,p.color.z,p.color.w*alpha},p.size*size_scale};
        }
        gl::Enable(GL_BLEND);
        gl::BlendFunc(GL_SRC_ALPHA, additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
        gl::BufferSubData(gl::array_buffer,
            static_cast<gl::GLintptr>(first*sizeof(GpuParticle)),
            static_cast<gl::GLsizeiptr>(count*sizeof(GpuParticle)),
            gpu_.data()+first);
        gl::DrawArrays(GL_POINTS,static_cast<GLint>(first),static_cast<GLsizei>(count));
    };

    bool drew_fire=false;
    bool drew_motes=false;
    bool drew_mist=false;
    for(const auto& emitter:scene.particle_emitters){
        if(!emitter.visible) continue;
        const float intensity=emitter.intensity*frame.controls.particle_strength;
        switch(emitter.kind){
        case scene::ParticleEmitterKind::fire:
            upload_and_draw(0u,fire_count,emitter.transform.position,emitter.size_scale,intensity,true);
            drew_fire=true;
            break;
        case scene::ParticleEmitterKind::motes:
            upload_and_draw(fire_count,mote_count,emitter.transform.position,emitter.size_scale,intensity,true);
            drew_motes=true;
            break;
        case scene::ParticleEmitterKind::mist:
            upload_and_draw(fire_count+mote_count,mist_count,emitter.transform.position,emitter.size_scale,intensity,false);
            drew_mist=true;
            break;
        }
    }

    if(scene.particle_emitters.empty()){
        upload_and_draw(0u,fire_count,{-15.0f,.20f,8.0f},1.0f,frame.controls.particle_strength,true);
        upload_and_draw(fire_count,mote_count,{16.0f,.18f,5.0f},1.0f,frame.controls.particle_strength,true);
        upload_and_draw(fire_count+mote_count,mist_count,{16.0f,.12f,5.0f},1.0f,frame.controls.particle_strength,false);
    } else {
        if(!drew_fire) upload_and_draw(0u,fire_count,{-15.0f,.20f,8.0f},1.0f,0.0f,true);
        if(!drew_motes) upload_and_draw(fire_count,mote_count,{16.0f,.18f,5.0f},1.0f,0.0f,true);
        if(!drew_mist) upload_and_draw(fire_count+mote_count,mist_count,{16.0f,.12f,5.0f},1.0f,0.0f,false);
    }

    gl::Disable(GL_BLEND);
    gl::DepthMask(GL_TRUE);
    gl::Disable(gl::program_point_size);
}
}
