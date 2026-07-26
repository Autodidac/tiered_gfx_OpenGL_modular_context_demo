#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#ifndef APIENTRY
#define APIENTRY
#endif
#endif

using GLenum = std::uint32_t;
using GLboolean = std::uint8_t;
using GLbitfield = std::uint32_t;
using GLvoid = void;
using GLbyte = std::int8_t;
using GLshort = std::int16_t;
using GLint = std::int32_t;
using GLsizei = std::int32_t;
using GLubyte = std::uint8_t;
using GLushort = std::uint16_t;
using GLuint = std::uint32_t;
using GLfloat = float;
using GLdouble = double;

#ifndef GL_FALSE
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_NONE 0
#define GL_NO_ERROR 0
#define GL_POINTS 0x0000
#define GL_TRIANGLES 0x0004
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_AND_BACK 0x0408
#define GL_CULL_FACE 0x0B44
#define GL_BLEND 0x0BE2
#define GL_DEPTH_TEST 0x0B71
#define GL_LESS 0x0201
#define GL_LEQUAL 0x0203
#define GL_CCW 0x0901
#define GL_LINE 0x1B01
#define GL_FILL 0x1B02
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_DEPTH_COMPONENT 0x1902
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_REPEAT 0x2901
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE 1
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_POLYGON_OFFSET_FILL 0x8037
#endif

namespace epoch::render::gl {

using GLchar = char;
using GLsizeiptr = std::ptrdiff_t;
using GLintptr = std::ptrdiff_t;

inline constexpr GLenum array_buffer = 0x8892;
inline constexpr GLenum element_array_buffer = 0x8893;
inline constexpr GLenum static_draw = 0x88E4;
inline constexpr GLenum dynamic_draw = 0x88E8;
inline constexpr GLenum vertex_shader = 0x8B31;
inline constexpr GLenum fragment_shader = 0x8B30;
inline constexpr GLenum geometry_shader = 0x8DD9;
inline constexpr GLenum tess_control_shader = 0x8E88;
inline constexpr GLenum tess_evaluation_shader = 0x8E87;
inline constexpr GLenum compute_shader = 0x91B9;
inline constexpr GLenum patches = 0x000E;
inline constexpr GLenum patch_vertices = 0x8E72;
inline constexpr GLenum draw_indirect_buffer = 0x8F3F;
inline constexpr GLenum shader_storage_buffer = 0x90D2;
inline constexpr GLenum transform_feedback_buffer = 0x8C8E;
inline constexpr GLenum rasterizer_discard = 0x8C89;
inline constexpr GLenum r32ui = 0x8236;
inline constexpr GLenum red_integer = 0x8D94;
inline constexpr GLenum rg16f = 0x822F;
inline constexpr GLenum rg = 0x8227;
inline constexpr GLenum depth24_stencil8 = 0x88F0;
inline constexpr GLenum depth_stencil_attachment = 0x821A;
inline constexpr GLenum depth_stencil = 0x84F9;
inline constexpr GLenum unsigned_int_24_8 = 0x84FA;
inline constexpr GLenum stencil_test = 0x0B90;
inline constexpr GLenum time_elapsed = 0x88BF;
inline constexpr GLenum query_result = 0x8866;
inline constexpr GLenum query_result_available = 0x8867;
inline constexpr GLenum shader_storage_barrier_bit = 0x2000;
inline constexpr GLenum vertex_attrib_array_barrier_bit = 0x00000001;
inline constexpr GLenum compile_status = 0x8B81;
inline constexpr GLenum link_status = 0x8B82;
inline constexpr GLenum info_log_length = 0x8B84;
inline constexpr GLenum texture0 = 0x84C0;
inline constexpr GLenum texture_cube_map = 0x8513;
inline constexpr GLenum texture_cube_map_positive_x = 0x8515;
inline constexpr GLenum texture_wrap_r = 0x8072;
inline constexpr GLenum framebuffer = 0x8D40;
inline constexpr GLenum read_framebuffer = 0x8CA8;
inline constexpr GLenum draw_framebuffer = 0x8CA9;
inline constexpr GLenum renderbuffer = 0x8D41;
inline constexpr GLenum color_attachment0 = 0x8CE0;
inline constexpr GLenum color_attachment1 = 0x8CE1;
inline constexpr GLenum color_attachment2 = 0x8CE2;
inline constexpr GLenum depth_attachment = 0x8D00;
inline constexpr GLenum framebuffer_complete = 0x8CD5;
inline constexpr GLenum depth_component24 = 0x81A6;
inline constexpr GLenum depth_component32f = 0x8CAC;
inline constexpr GLenum rgba8 = 0x8058;
inline constexpr GLenum srgb8_alpha8 = 0x8C43;
inline constexpr GLenum rgb8 = 0x8051;
inline constexpr GLenum r8 = 0x8229;
inline constexpr GLenum red = 0x1903;
inline constexpr GLenum srgb8 = 0x8C41;
inline constexpr GLenum rgba16f = 0x881A;
inline constexpr GLenum rgb16f = 0x881B;
inline constexpr GLenum clamp_to_edge = 0x812F;
inline constexpr GLenum clamp_to_border = 0x812D;
inline constexpr GLenum texture_border_color = 0x1004;
inline constexpr GLenum texture_max_anisotropy_ext = 0x84FE;
inline constexpr GLenum max_texture_max_anisotropy_ext = 0x84FF;
inline constexpr GLenum framebuffer_srgb = 0x8DB9;
inline constexpr GLenum multisample = 0x809D;
inline constexpr GLenum sample_alpha_to_coverage = 0x809E;
inline constexpr GLenum debug_output = 0x92E0;
inline constexpr GLenum debug_output_synchronous = 0x8242;
inline constexpr GLenum program_point_size = 0x8642;

using EnableFn = void(APIENTRY*)(GLenum);
using DisableFn = void(APIENTRY*)(GLenum);
using ClearFn = void(APIENTRY*)(GLbitfield);
using ClearColorFn = void(APIENTRY*)(GLfloat, GLfloat, GLfloat, GLfloat);
using CullFaceFn = void(APIENTRY*)(GLenum);
using FrontFaceFn = void(APIENTRY*)(GLenum);
using DepthFuncFn = void(APIENTRY*)(GLenum);
using DepthMaskFn = void(APIENTRY*)(GLboolean);
using BlendFuncFn = void(APIENTRY*)(GLenum, GLenum);
using DrawArraysFn = void(APIENTRY*)(GLenum, GLint, GLsizei);
using DrawElementsFn = void(APIENTRY*)(GLenum, GLsizei, GLenum, const void*);
using DrawBufferFn = void(APIENTRY*)(GLenum);
using ReadBufferFn = void(APIENTRY*)(GLenum);
using ViewportFn = void(APIENTRY*)(GLint, GLint, GLsizei, GLsizei);
using LineWidthFn = void(APIENTRY*)(GLfloat);
using PolygonModeFn = void(APIENTRY*)(GLenum, GLenum);
using PolygonOffsetFn = void(APIENTRY*)(GLfloat, GLfloat);
using GenTexturesFn = void(APIENTRY*)(GLsizei, GLuint*);
using DeleteTexturesFn = void(APIENTRY*)(GLsizei, const GLuint*);
using BindTextureFn = void(APIENTRY*)(GLenum, GLuint);
using TexImage2DFn = void(APIENTRY*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
using TexParameteriFn = void(APIENTRY*)(GLenum, GLenum, GLint);
using TexParameterfFn = void(APIENTRY*)(GLenum, GLenum, GLfloat);
using TexParameterfvFn = void(APIENTRY*)(GLenum, GLenum, const GLfloat*);
using PixelStoreiFn = void(APIENTRY*)(GLenum, GLint);
using GetIntegervFn = void(APIENTRY*)(GLenum, GLint*);
using GetFloatvFn = void(APIENTRY*)(GLenum, GLfloat*);
using GetErrorFn = GLenum(APIENTRY*)();

using CreateShaderFn = GLuint(APIENTRY*)(GLenum);
using ShaderSourceFn = void(APIENTRY*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
using CompileShaderFn = void(APIENTRY*)(GLuint);
using GetShaderivFn = void(APIENTRY*)(GLuint, GLenum, GLint*);
using GetShaderInfoLogFn = void(APIENTRY*)(GLuint, GLsizei, GLsizei*, GLchar*);
using DeleteShaderFn = void(APIENTRY*)(GLuint);
using CreateProgramFn = GLuint(APIENTRY*)();
using AttachShaderFn = void(APIENTRY*)(GLuint, GLuint);
using LinkProgramFn = void(APIENTRY*)(GLuint);
using GetProgramivFn = void(APIENTRY*)(GLuint, GLenum, GLint*);
using GetProgramInfoLogFn = void(APIENTRY*)(GLuint, GLsizei, GLsizei*, GLchar*);
using DeleteProgramFn = void(APIENTRY*)(GLuint);
using UseProgramFn = void(APIENTRY*)(GLuint);
using GetUniformLocationFn = GLint(APIENTRY*)(GLuint, const GLchar*);
using UniformMatrix4fvFn = void(APIENTRY*)(GLint, GLsizei, GLboolean, const GLfloat*);
using Uniform4fFn = void(APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
using Uniform3fFn = void(APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat);
using Uniform2fFn = void(APIENTRY*)(GLint, GLfloat, GLfloat);
using Uniform1fFn = void(APIENTRY*)(GLint, GLfloat);
using Uniform1iFn = void(APIENTRY*)(GLint, GLint);
using Uniform1uiFn = void(APIENTRY*)(GLint, GLuint);
using BufferSubDataFn = void(APIENTRY*)(GLenum, GLintptr, GLsizeiptr, const void*);
using VertexAttribIPointerFn = void(APIENTRY*)(GLuint, GLint, GLenum, GLsizei, const void*);
using VertexAttribDivisorFn = void(APIENTRY*)(GLuint, GLuint);
using PatchParameteriFn = void(APIENTRY*)(GLenum, GLint);
using DrawElementsIndirectFn = void(APIENTRY*)(GLenum, GLenum, const void*);
using CopyTexSubImage2DFn = void(APIENTRY*)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
using BlitFramebufferFn = void(APIENTRY*)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
using ClearBufferuivFn = void(APIENTRY*)(GLenum, GLint, const GLuint*);
using BindBufferBaseFn = void(APIENTRY*)(GLenum, GLuint, GLuint);
using DispatchComputeFn = void(APIENTRY*)(GLuint, GLuint, GLuint);
using MemoryBarrierFn = void(APIENTRY*)(GLbitfield);
using GenQueriesFn = void(APIENTRY*)(GLsizei, GLuint*);
using DeleteQueriesFn = void(APIENTRY*)(GLsizei, const GLuint*);
using BeginQueryFn = void(APIENTRY*)(GLenum, GLuint);
using EndQueryFn = void(APIENTRY*)(GLenum);
using GetQueryObjectui64vFn = void(APIENTRY*)(GLuint, GLenum, unsigned long long*);
using GetQueryObjectivFn = void(APIENTRY*)(GLuint, GLenum, GLint*);
using GenVertexArraysFn = void(APIENTRY*)(GLsizei, GLuint*);
using BindVertexArrayFn = void(APIENTRY*)(GLuint);
using DeleteVertexArraysFn = void(APIENTRY*)(GLsizei, const GLuint*);
using GenBuffersFn = void(APIENTRY*)(GLsizei, GLuint*);
using BindBufferFn = void(APIENTRY*)(GLenum, GLuint);
using BufferDataFn = void(APIENTRY*)(GLenum, GLsizeiptr, const void*, GLenum);
using DeleteBuffersFn = void(APIENTRY*)(GLsizei, const GLuint*);
using EnableVertexAttribArrayFn = void(APIENTRY*)(GLuint);
using VertexAttribPointerFn = void(APIENTRY*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
using ActiveTextureFn = void(APIENTRY*)(GLenum);
using GenerateMipmapFn = void(APIENTRY*)(GLenum);
using GenFramebuffersFn = void(APIENTRY*)(GLsizei, GLuint*);
using BindFramebufferFn = void(APIENTRY*)(GLenum, GLuint);
using FramebufferTexture2DFn = void(APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);
using CheckFramebufferStatusFn = GLenum(APIENTRY*)(GLenum);
using DeleteFramebuffersFn = void(APIENTRY*)(GLsizei, const GLuint*);
using DrawBuffersFn = void(APIENTRY*)(GLsizei, const GLenum*);
using GenRenderbuffersFn = void(APIENTRY*)(GLsizei, GLuint*);
using BindRenderbufferFn = void(APIENTRY*)(GLenum, GLuint);
using RenderbufferStorageFn = void(APIENTRY*)(GLenum, GLenum, GLsizei, GLsizei);
using FramebufferRenderbufferFn = void(APIENTRY*)(GLenum, GLenum, GLenum, GLuint);
using DeleteRenderbuffersFn = void(APIENTRY*)(GLsizei, const GLuint*);
using DrawElementsInstancedFn = void(APIENTRY*)(GLenum, GLsizei, GLenum, const void*, GLsizei);
using DebugMessageCallbackFn = void(APIENTRY*)(void(APIENTRY*)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void*), const void*);

extern EnableFn Enable;
extern DisableFn Disable;
extern ClearFn Clear;
extern ClearColorFn ClearColor;
extern CullFaceFn CullFace;
extern FrontFaceFn FrontFace;
extern DepthFuncFn DepthFunc;
extern DepthMaskFn DepthMask;
extern BlendFuncFn BlendFunc;
extern DrawArraysFn DrawArrays;
extern DrawElementsFn DrawElements;
extern DrawBufferFn DrawBuffer;
extern ReadBufferFn ReadBuffer;
extern ViewportFn Viewport;
extern LineWidthFn LineWidth;
extern PolygonModeFn PolygonMode;
extern PolygonOffsetFn PolygonOffset;
extern GenTexturesFn GenTextures;
extern DeleteTexturesFn DeleteTextures;
extern BindTextureFn BindTexture;
extern TexImage2DFn TexImage2D;
extern TexParameteriFn TexParameteri;
extern TexParameterfFn TexParameterf;
extern TexParameterfvFn TexParameterfv;
extern PixelStoreiFn PixelStorei;
extern GetIntegervFn GetIntegerv;
extern GetFloatvFn GetFloatv;
extern GetErrorFn GetError;

extern CreateShaderFn CreateShader;
extern ShaderSourceFn ShaderSource;
extern CompileShaderFn CompileShader;
extern GetShaderivFn GetShaderiv;
extern GetShaderInfoLogFn GetShaderInfoLog;
extern DeleteShaderFn DeleteShader;
extern CreateProgramFn CreateProgram;
extern AttachShaderFn AttachShader;
extern LinkProgramFn LinkProgram;
extern GetProgramivFn GetProgramiv;
extern GetProgramInfoLogFn GetProgramInfoLog;
extern DeleteProgramFn DeleteProgram;
extern UseProgramFn UseProgram;
extern GetUniformLocationFn GetUniformLocation;
extern UniformMatrix4fvFn UniformMatrix4fv;
extern Uniform4fFn Uniform4f;
extern Uniform3fFn Uniform3f;
extern Uniform2fFn Uniform2f;
extern Uniform1fFn Uniform1f;
extern Uniform1iFn Uniform1i;
extern Uniform1uiFn Uniform1ui;
extern BufferSubDataFn BufferSubData;
extern VertexAttribIPointerFn VertexAttribIPointer;
extern VertexAttribDivisorFn VertexAttribDivisor;
extern PatchParameteriFn PatchParameteri;
extern DrawElementsIndirectFn DrawElementsIndirect;
extern CopyTexSubImage2DFn CopyTexSubImage2D;
extern BlitFramebufferFn BlitFramebuffer;
extern ClearBufferuivFn ClearBufferuiv;
extern BindBufferBaseFn BindBufferBase;
extern DispatchComputeFn DispatchCompute;
extern MemoryBarrierFn MemoryBarrier;
extern GenQueriesFn GenQueries;
extern DeleteQueriesFn DeleteQueries;
extern BeginQueryFn BeginQuery;
extern EndQueryFn EndQuery;
extern GetQueryObjectui64vFn GetQueryObjectui64v;
extern GetQueryObjectivFn GetQueryObjectiv;
extern GenVertexArraysFn GenVertexArrays;
extern BindVertexArrayFn BindVertexArray;
extern DeleteVertexArraysFn DeleteVertexArrays;
extern GenBuffersFn GenBuffers;
extern BindBufferFn BindBuffer;
extern BufferDataFn BufferData;
extern DeleteBuffersFn DeleteBuffers;
extern EnableVertexAttribArrayFn EnableVertexAttribArray;
extern VertexAttribPointerFn VertexAttribPointer;
extern ActiveTextureFn ActiveTexture;
extern GenerateMipmapFn GenerateMipmap;
extern GenFramebuffersFn GenFramebuffers;
extern BindFramebufferFn BindFramebuffer;
extern FramebufferTexture2DFn FramebufferTexture2D;
extern CheckFramebufferStatusFn CheckFramebufferStatus;
extern DeleteFramebuffersFn DeleteFramebuffers;
extern DrawBuffersFn DrawBuffers;
extern GenRenderbuffersFn GenRenderbuffers;
extern BindRenderbufferFn BindRenderbuffer;
extern RenderbufferStorageFn RenderbufferStorage;
extern FramebufferRenderbufferFn FramebufferRenderbuffer;
extern DeleteRenderbuffersFn DeleteRenderbuffers;
extern DrawElementsInstancedFn DrawElementsInstanced;
extern DebugMessageCallbackFn DebugMessageCallback;

[[nodiscard]] void* proc_address(const char* name) noexcept;
void load_all();

} // namespace epoch::render::gl
