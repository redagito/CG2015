#pragma once
#include "graphics/renderer/CDeferredRenderer.h"

class PostProcessTestRenderer : public CDeferredRenderer
{
  public:
    using CDeferredRenderer::geometryPass;
    using CDeferredRenderer::lensFlarePass2;
    using CDeferredRenderer::postProcessPass;
};
