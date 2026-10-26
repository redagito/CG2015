#pragma once
#include "graphics/renderer/core/RendererCoreConfig.h"
#include <array>

// Restores the caller's state at the renderer boundary. Individual passes still
// declare their own depth/blend/cull policy rather than inheriting prior passes.
class CScopedRenderState
{
    struct Unit
    {
        GLint texture2D, cube, sampler;
    };
    std::array<Unit, 7> units{};
    GLint drawFbo, readFbo, program, vao, viewport[4], active, depthFunc, cullFace, frontFace;
    GLint srcRgb, dstRgb, srcAlpha, dstAlpha, eqRgb, eqAlpha, polygon[2];
    GLboolean depthMask, colorMask[4];
    static constexpr std::array<GLenum, 9> caps = {GL_DEPTH_TEST,         GL_BLEND,
                                                   GL_CULL_FACE,          GL_SCISSOR_TEST,
                                                   GL_STENCIL_TEST,       GL_FRAMEBUFFER_SRGB,
                                                   GL_RASTERIZER_DISCARD, GL_POLYGON_OFFSET_FILL,
                                                   GL_DEPTH_CLAMP};
    std::array<GLboolean, caps.size()> enabled{};

  public:
    CScopedRenderState();
    ~CScopedRenderState();
    CScopedRenderState(const CScopedRenderState &) = delete;
};
