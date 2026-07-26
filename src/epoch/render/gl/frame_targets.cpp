#include "epoch/render/gl/frame_targets.hpp"
#include <algorithm>
#include <stdexcept>

namespace epoch::render::gl {

ShadowTarget::ShadowTarget(int size):size_{size}{
    GenFramebuffers(1,&fbo_);gl::GenTextures(1,&depth_);gl::BindTexture(GL_TEXTURE_2D,depth_);
    gl::TexImage2D(GL_TEXTURE_2D,0,depth_component32f,size_,size_,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
    gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,clamp_to_border);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,clamp_to_border);
    constexpr float border[]{1,1,1,1};gl::TexParameterfv(GL_TEXTURE_2D,texture_border_color,border);
    BindFramebuffer(framebuffer,fbo_);FramebufferTexture2D(framebuffer,depth_attachment,GL_TEXTURE_2D,depth_,0);gl::DrawBuffer(GL_NONE);gl::ReadBuffer(GL_NONE);
    if(CheckFramebufferStatus(framebuffer)!=framebuffer_complete)throw std::runtime_error("Shadow framebuffer is incomplete");BindFramebuffer(framebuffer,0);
}
ShadowTarget::~ShadowTarget(){if(depth_)gl::DeleteTextures(1,&depth_);if(fbo_)DeleteFramebuffers(1,&fbo_);}
void ShadowTarget::bind_for_write()const noexcept{BindFramebuffer(framebuffer,fbo_);gl::Viewport(0,0,size_,size_);gl::Clear(GL_DEPTH_BUFFER_BIT);}
void ShadowTarget::bind_depth(int unit)const noexcept{ActiveTexture(texture0+unit);gl::BindTexture(GL_TEXTURE_2D,depth_);}

HdrTarget::~HdrTarget(){destroy();}
void HdrTarget::destroy()noexcept{if(depth_)gl::DeleteTextures(1,&depth_);if(color_[0]||color_[1])gl::DeleteTextures(2,color_.data());if(fbo_)DeleteFramebuffers(1,&fbo_);depth_=fbo_=0;color_={};width_=height_=0;}
void HdrTarget::resize(int width,int height){width=std::max(1,width);height=std::max(1,height);if(width==width_&&height==height_)return;destroy();width_=width;height_=height;
    GenFramebuffers(1,&fbo_);BindFramebuffer(framebuffer,fbo_);gl::GenTextures(2,color_.data());
    for(int i=0;i<2;++i){gl::BindTexture(GL_TEXTURE_2D,color_[i]);gl::TexImage2D(GL_TEXTURE_2D,0,rgba16f,width_,height_,0,GL_RGBA,GL_FLOAT,nullptr);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,clamp_to_edge);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,clamp_to_edge);FramebufferTexture2D(framebuffer,color_attachment0+i,GL_TEXTURE_2D,color_[i],0);}
    const GLenum buffers[]{color_attachment0,color_attachment1};DrawBuffers(2,buffers);
    gl::GenTextures(1,&depth_);gl::BindTexture(GL_TEXTURE_2D,depth_);
    gl::TexImage2D(GL_TEXTURE_2D,0,depth_component24,width_,height_,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,nullptr);
    gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,clamp_to_edge);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,clamp_to_edge);
    FramebufferTexture2D(framebuffer,depth_attachment,GL_TEXTURE_2D,depth_,0);
    if(CheckFramebufferStatus(framebuffer)!=framebuffer_complete)throw std::runtime_error("HDR framebuffer is incomplete");BindFramebuffer(framebuffer,0);
}
void HdrTarget::bind_for_write()const noexcept{BindFramebuffer(framebuffer,fbo_);gl::Viewport(0,0,width_,height_);}
void HdrTarget::bind_scene(int unit)const noexcept{ActiveTexture(texture0+unit);gl::BindTexture(GL_TEXTURE_2D,color_[0]);}
void HdrTarget::bind_bright(int unit)const noexcept{ActiveTexture(texture0+unit);gl::BindTexture(GL_TEXTURE_2D,color_[1]);}
void HdrTarget::bind_depth(int unit)const noexcept{ActiveTexture(texture0+unit);gl::BindTexture(GL_TEXTURE_2D,depth_);}

BlurTargets::~BlurTargets(){destroy();}
void BlurTargets::destroy()noexcept{if(texture_[0]||texture_[1])gl::DeleteTextures(2,texture_.data());if(fbo_[0]||fbo_[1])DeleteFramebuffers(2,fbo_.data());texture_={};fbo_={};width_=height_=0;}
void BlurTargets::resize(int width,int height){width=std::max(1,width/2);height=std::max(1,height/2);if(width==width_&&height==height_)return;destroy();width_=width;height_=height;GenFramebuffers(2,fbo_.data());gl::GenTextures(2,texture_.data());
    for(int i=0;i<2;++i){BindFramebuffer(framebuffer,fbo_[i]);gl::BindTexture(GL_TEXTURE_2D,texture_[i]);gl::TexImage2D(GL_TEXTURE_2D,0,rgba16f,width_,height_,0,GL_RGBA,GL_FLOAT,nullptr);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,clamp_to_edge);gl::TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,clamp_to_edge);FramebufferTexture2D(framebuffer,color_attachment0,GL_TEXTURE_2D,texture_[i],0);if(CheckFramebufferStatus(framebuffer)!=framebuffer_complete)throw std::runtime_error("Blur framebuffer is incomplete");}BindFramebuffer(framebuffer,0);
}
void BlurTargets::bind_for_write(int index)const noexcept{BindFramebuffer(framebuffer,fbo_[index&1]);gl::Viewport(0,0,width_,height_);}
void BlurTargets::bind_texture(int index,int unit)const noexcept{ActiveTexture(texture0+unit);gl::BindTexture(GL_TEXTURE_2D,texture_[index&1]);}

} // namespace epoch::render::gl
