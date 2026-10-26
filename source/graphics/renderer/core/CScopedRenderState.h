#pragma once
#include "graphics/renderer/core/RendererCoreConfig.h"
#include <array>

// Restores the caller's state at the renderer boundary. Individual passes still
// declare their own depth/blend/cull policy rather than inheriting prior passes.
class CScopedRenderState
{
    struct Unit { GLint texture2D, cube, sampler; };
    std::array<Unit, 7> units{};
    GLint drawFbo, readFbo, program, vao, viewport[4], active, depthFunc, cullFace, frontFace;
    GLint srcRgb, dstRgb, srcAlpha, dstAlpha, eqRgb, eqAlpha, polygon[2];
    GLboolean depthMask, colorMask[4];
    static constexpr std::array<GLenum, 9> caps = {GL_DEPTH_TEST, GL_BLEND, GL_CULL_FACE, GL_SCISSOR_TEST, GL_STENCIL_TEST, GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD, GL_POLYGON_OFFSET_FILL, GL_DEPTH_CLAMP};
    std::array<GLboolean, caps.size()> enabled{};
public:
    CScopedRenderState()
    {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&drawFbo); glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&readFbo);
        glGetIntegerv(GL_CURRENT_PROGRAM,&program); glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        glGetIntegerv(GL_VIEWPORT,viewport); glGetIntegerv(GL_ACTIVE_TEXTURE,&active);
        glGetIntegerv(GL_DEPTH_FUNC,&depthFunc); glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask); glGetBooleanv(GL_COLOR_WRITEMASK,colorMask);
        glGetIntegerv(GL_CULL_FACE_MODE,&cullFace); glGetIntegerv(GL_FRONT_FACE,&frontFace); glGetIntegerv(GL_POLYGON_MODE,polygon);
        glGetIntegerv(GL_BLEND_SRC_RGB,&srcRgb); glGetIntegerv(GL_BLEND_DST_RGB,&dstRgb); glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcAlpha); glGetIntegerv(GL_BLEND_DST_ALPHA,&dstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB,&eqRgb); glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&eqAlpha);
        for(size_t i=0;i<caps.size();++i) enabled[i]=glIsEnabled(caps[i]);
        for(size_t i=0;i<units.size();++i) { glActiveTexture(GL_TEXTURE0+static_cast<GLenum>(i)); glGetIntegerv(GL_TEXTURE_BINDING_2D,&units[i].texture2D); glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP,&units[i].cube); glGetIntegeri_v(GL_SAMPLER_BINDING,static_cast<GLuint>(i),&units[i].sampler); }
        glActiveTexture(active);
    }
    ~CScopedRenderState()
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,drawFbo); glBindFramebuffer(GL_READ_FRAMEBUFFER,readFbo);
        glUseProgram(program); glBindVertexArray(vao); glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
        glDepthFunc(depthFunc); glDepthMask(depthMask); glColorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);
        glCullFace(cullFace); glFrontFace(frontFace); glPolygonMode(GL_FRONT_AND_BACK,polygon[0]);
        glBlendFuncSeparate(srcRgb,dstRgb,srcAlpha,dstAlpha); glBlendEquationSeparate(eqRgb,eqAlpha);
        for(size_t i=0;i<caps.size();++i) { if(enabled[i]) glEnable(caps[i]); else glDisable(caps[i]); }
        for(size_t i=0;i<units.size();++i) { glActiveTexture(GL_TEXTURE0+static_cast<GLenum>(i)); glBindTexture(GL_TEXTURE_2D,units[i].texture2D); glBindTexture(GL_TEXTURE_CUBE_MAP,units[i].cube); glBindSampler(static_cast<GLuint>(i),units[i].sampler); }
        glActiveTexture(active);
    }
    CScopedRenderState(const CScopedRenderState&) = delete;
};
