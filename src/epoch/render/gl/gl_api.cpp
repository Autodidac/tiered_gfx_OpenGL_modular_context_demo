#include "epoch/render/gl/gl_api.hpp"
#include <stdexcept>
#include <string>
#if !defined(_WIN32)
#include <dlfcn.h>
#endif

namespace epoch::render::gl {

EnableFn Enable{};
DisableFn Disable{};
ClearFn Clear{};
ClearColorFn ClearColor{};
CullFaceFn CullFace{};
FrontFaceFn FrontFace{};
DepthFuncFn DepthFunc{};
DepthMaskFn DepthMask{};
BlendFuncFn BlendFunc{};
DrawArraysFn DrawArrays{};
DrawElementsFn DrawElements{};
DrawBufferFn DrawBuffer{};
ReadBufferFn ReadBuffer{};
ViewportFn Viewport{};
LineWidthFn LineWidth{};
PolygonModeFn PolygonMode{};
PolygonOffsetFn PolygonOffset{};
GenTexturesFn GenTextures{};
DeleteTexturesFn DeleteTextures{};
BindTextureFn BindTexture{};
TexImage2DFn TexImage2D{};
TexParameteriFn TexParameteri{};
TexParameterfFn TexParameterf{};
TexParameterfvFn TexParameterfv{};
PixelStoreiFn PixelStorei{};
GetIntegervFn GetIntegerv{};
GetFloatvFn GetFloatv{};
GetErrorFn GetError{};

CreateShaderFn CreateShader{};
ShaderSourceFn ShaderSource{};
CompileShaderFn CompileShader{};
GetShaderivFn GetShaderiv{};
GetShaderInfoLogFn GetShaderInfoLog{};
DeleteShaderFn DeleteShader{};
CreateProgramFn CreateProgram{};
AttachShaderFn AttachShader{};
LinkProgramFn LinkProgram{};
GetProgramivFn GetProgramiv{};
GetProgramInfoLogFn GetProgramInfoLog{};
DeleteProgramFn DeleteProgram{};
UseProgramFn UseProgram{};
GetUniformLocationFn GetUniformLocation{};
UniformMatrix4fvFn UniformMatrix4fv{};
Uniform4fFn Uniform4f{};
Uniform3fFn Uniform3f{};
Uniform2fFn Uniform2f{};
Uniform1fFn Uniform1f{};
Uniform1iFn Uniform1i{};
Uniform1uiFn Uniform1ui{};
BufferSubDataFn BufferSubData{};
VertexAttribIPointerFn VertexAttribIPointer{};
VertexAttribDivisorFn VertexAttribDivisor{};
PatchParameteriFn PatchParameteri{};
DrawElementsIndirectFn DrawElementsIndirect{};
CopyTexSubImage2DFn CopyTexSubImage2D{};
BlitFramebufferFn BlitFramebuffer{};
ClearBufferuivFn ClearBufferuiv{};
BindBufferBaseFn BindBufferBase{};
DispatchComputeFn DispatchCompute{};
MemoryBarrierFn MemoryBarrier{};
GenQueriesFn GenQueries{};
DeleteQueriesFn DeleteQueries{};
BeginQueryFn BeginQuery{};
EndQueryFn EndQuery{};
GetQueryObjectui64vFn GetQueryObjectui64v{};
GetQueryObjectivFn GetQueryObjectiv{};
GenVertexArraysFn GenVertexArrays{};
BindVertexArrayFn BindVertexArray{};
DeleteVertexArraysFn DeleteVertexArrays{};
GenBuffersFn GenBuffers{};
BindBufferFn BindBuffer{};
BufferDataFn BufferData{};
DeleteBuffersFn DeleteBuffers{};
EnableVertexAttribArrayFn EnableVertexAttribArray{};
VertexAttribPointerFn VertexAttribPointer{};
ActiveTextureFn ActiveTexture{};
GenerateMipmapFn GenerateMipmap{};
GenFramebuffersFn GenFramebuffers{};
BindFramebufferFn BindFramebuffer{};
FramebufferTexture2DFn FramebufferTexture2D{};
CheckFramebufferStatusFn CheckFramebufferStatus{};
DeleteFramebuffersFn DeleteFramebuffers{};
DrawBuffersFn DrawBuffers{};
GenRenderbuffersFn GenRenderbuffers{};
BindRenderbufferFn BindRenderbuffer{};
RenderbufferStorageFn RenderbufferStorage{};
FramebufferRenderbufferFn FramebufferRenderbuffer{};
DeleteRenderbuffersFn DeleteRenderbuffers{};
DrawElementsInstancedFn DrawElementsInstanced{};
DebugMessageCallbackFn DebugMessageCallback{};

void* proc_address(const char* name) noexcept {
#if defined(_WIN32)
    void* proc = reinterpret_cast<void*>(wglGetProcAddress(name));
    if (proc == nullptr || proc == reinterpret_cast<void*>(1) || proc == reinterpret_cast<void*>(2) ||
        proc == reinterpret_cast<void*>(3) || proc == reinterpret_cast<void*>(-1)) {
        static HMODULE module = LoadLibraryW(L"opengl32.dll");
        proc = module ? reinterpret_cast<void*>(GetProcAddress(module, name)) : nullptr;
    }
    return proc;
#else
    using GlxGetProcAddressFn = void*(*)(const unsigned char*);
    static void* module = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);
    static auto get_proc = module
        ? reinterpret_cast<GlxGetProcAddressFn>(dlsym(module, "glXGetProcAddressARB"))
        : nullptr;
    void* proc = get_proc ? get_proc(reinterpret_cast<const unsigned char*>(name)) : nullptr;
    if (!proc && module) proc = dlsym(module, name);
    return proc;
#endif
}

namespace {
template <typename T>
void require(T& target, const char* name) {
    target = reinterpret_cast<T>(proc_address(name));
    if (!target) throw std::runtime_error(std::string{"Missing OpenGL function: "} + name);
}

template <typename T>
void optional(T& target, const char* name) noexcept {
    target = reinterpret_cast<T>(proc_address(name));
}
}

void load_all() {
#define EPOCH_GL_REQUIRED(name) require(name, "gl" #name)
    EPOCH_GL_REQUIRED(Enable); EPOCH_GL_REQUIRED(Disable); EPOCH_GL_REQUIRED(Clear); EPOCH_GL_REQUIRED(ClearColor);
    EPOCH_GL_REQUIRED(CullFace); EPOCH_GL_REQUIRED(FrontFace); EPOCH_GL_REQUIRED(DepthFunc); EPOCH_GL_REQUIRED(DepthMask);
    EPOCH_GL_REQUIRED(BlendFunc); EPOCH_GL_REQUIRED(DrawArrays); EPOCH_GL_REQUIRED(DrawElements);
    EPOCH_GL_REQUIRED(DrawBuffer); EPOCH_GL_REQUIRED(ReadBuffer); EPOCH_GL_REQUIRED(Viewport); EPOCH_GL_REQUIRED(LineWidth);
    EPOCH_GL_REQUIRED(PolygonMode); EPOCH_GL_REQUIRED(PolygonOffset); EPOCH_GL_REQUIRED(GenTextures);
    EPOCH_GL_REQUIRED(DeleteTextures); EPOCH_GL_REQUIRED(BindTexture); EPOCH_GL_REQUIRED(TexImage2D);
    EPOCH_GL_REQUIRED(TexParameteri); EPOCH_GL_REQUIRED(TexParameterf); EPOCH_GL_REQUIRED(TexParameterfv);
    EPOCH_GL_REQUIRED(PixelStorei); EPOCH_GL_REQUIRED(GetIntegerv); EPOCH_GL_REQUIRED(GetFloatv); EPOCH_GL_REQUIRED(GetError);
    EPOCH_GL_REQUIRED(CreateShader); EPOCH_GL_REQUIRED(ShaderSource); EPOCH_GL_REQUIRED(CompileShader);
    EPOCH_GL_REQUIRED(GetShaderiv); EPOCH_GL_REQUIRED(GetShaderInfoLog); EPOCH_GL_REQUIRED(DeleteShader);
    EPOCH_GL_REQUIRED(CreateProgram); EPOCH_GL_REQUIRED(AttachShader); EPOCH_GL_REQUIRED(LinkProgram);
    EPOCH_GL_REQUIRED(GetProgramiv); EPOCH_GL_REQUIRED(GetProgramInfoLog); EPOCH_GL_REQUIRED(DeleteProgram);
    EPOCH_GL_REQUIRED(UseProgram); EPOCH_GL_REQUIRED(GetUniformLocation); EPOCH_GL_REQUIRED(UniformMatrix4fv);
    EPOCH_GL_REQUIRED(Uniform4f); EPOCH_GL_REQUIRED(Uniform3f); EPOCH_GL_REQUIRED(Uniform2f);
    EPOCH_GL_REQUIRED(Uniform1f); EPOCH_GL_REQUIRED(Uniform1i); EPOCH_GL_REQUIRED(Uniform1ui); EPOCH_GL_REQUIRED(GenVertexArrays);
    EPOCH_GL_REQUIRED(BindVertexArray); EPOCH_GL_REQUIRED(DeleteVertexArrays); EPOCH_GL_REQUIRED(GenBuffers);
    EPOCH_GL_REQUIRED(BindBuffer); EPOCH_GL_REQUIRED(BufferData); EPOCH_GL_REQUIRED(BufferSubData); EPOCH_GL_REQUIRED(DeleteBuffers);
    EPOCH_GL_REQUIRED(EnableVertexAttribArray); EPOCH_GL_REQUIRED(VertexAttribPointer); EPOCH_GL_REQUIRED(VertexAttribIPointer); EPOCH_GL_REQUIRED(VertexAttribDivisor); EPOCH_GL_REQUIRED(ActiveTexture);
    EPOCH_GL_REQUIRED(GenerateMipmap); EPOCH_GL_REQUIRED(GenFramebuffers); EPOCH_GL_REQUIRED(BindFramebuffer);
    EPOCH_GL_REQUIRED(FramebufferTexture2D); EPOCH_GL_REQUIRED(CheckFramebufferStatus); EPOCH_GL_REQUIRED(DeleteFramebuffers);
    EPOCH_GL_REQUIRED(DrawBuffers); EPOCH_GL_REQUIRED(GenRenderbuffers); EPOCH_GL_REQUIRED(BindRenderbuffer);
    EPOCH_GL_REQUIRED(RenderbufferStorage); EPOCH_GL_REQUIRED(FramebufferRenderbuffer); EPOCH_GL_REQUIRED(DeleteRenderbuffers);
    EPOCH_GL_REQUIRED(DrawElementsInstanced); EPOCH_GL_REQUIRED(PatchParameteri); EPOCH_GL_REQUIRED(DrawElementsIndirect);
    EPOCH_GL_REQUIRED(CopyTexSubImage2D); EPOCH_GL_REQUIRED(BlitFramebuffer); EPOCH_GL_REQUIRED(ClearBufferuiv); EPOCH_GL_REQUIRED(BindBufferBase);
    EPOCH_GL_REQUIRED(DispatchCompute); EPOCH_GL_REQUIRED(MemoryBarrier); EPOCH_GL_REQUIRED(GenQueries);
    EPOCH_GL_REQUIRED(DeleteQueries); EPOCH_GL_REQUIRED(BeginQuery); EPOCH_GL_REQUIRED(EndQuery); EPOCH_GL_REQUIRED(GetQueryObjectui64v); EPOCH_GL_REQUIRED(GetQueryObjectiv);
#undef EPOCH_GL_REQUIRED
    optional(DebugMessageCallback, "glDebugMessageCallback");
}

} // namespace epoch::render::gl
