#pragma once
#include "graphics/renderer/CDeferredRenderer.h"

class RenderingTestProbe : public CDeferredRenderer
{
  public:
    using CDeferredRenderer::backgroundPass;
    using CDeferredRenderer::draw;
    using CDeferredRenderer::geometryPass;
    using CDeferredRenderer::godRayPass1;
    using CDeferredRenderer::illuminationPass;
    using CDeferredRenderer::lightPass;
    using CDeferredRenderer::transparencyPass;
};
