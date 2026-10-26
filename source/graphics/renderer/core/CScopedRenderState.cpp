#include "CScopedRenderState.h"

CScopedRenderState::CScopedRenderState()
{
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
    glGetIntegerv(GL_CULL_FACE_MODE, &cullFace);
    glGetIntegerv(GL_FRONT_FACE, &frontFace);
    glGetIntegerv(GL_POLYGON_MODE, polygon);
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &dstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &eqRgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &eqAlpha);
    for (size_t i = 0; i < caps.size(); ++i)
        enabled[i] = glIsEnabled(caps[i]);
    for (size_t i = 0; i < units.size(); ++i)
    {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &units[i].texture2D);
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &units[i].cube);
        glGetIntegeri_v(GL_SAMPLER_BINDING, static_cast<GLuint>(i), &units[i].sampler);
    }
    glActiveTexture(active);
}

CScopedRenderState::~CScopedRenderState()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
    glUseProgram(program);
    glBindVertexArray(vao);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDepthFunc(depthFunc);
    glDepthMask(depthMask);
    glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    glCullFace(cullFace);
    glFrontFace(frontFace);
    glPolygonMode(GL_FRONT_AND_BACK, polygon[0]);
    glBlendFuncSeparate(srcRgb, dstRgb, srcAlpha, dstAlpha);
    glBlendEquationSeparate(eqRgb, eqAlpha);
    for (size_t i = 0; i < caps.size(); ++i)
    {
        if (enabled[i])
            glEnable(caps[i]);
        else
            glDisable(caps[i]);
    }
    for (size_t i = 0; i < units.size(); ++i)
    {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
        glBindTexture(GL_TEXTURE_2D, units[i].texture2D);
        glBindTexture(GL_TEXTURE_CUBE_MAP, units[i].cube);
        glBindSampler(static_cast<GLuint>(i), units[i].sampler);
    }
    glActiveTexture(active);
}
