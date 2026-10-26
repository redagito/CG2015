#pragma once
#include "CFrameBuffer.h"
#include "graphics/SceneConfig.h"
#include "math/CTransformer.h"
#include "resource/ResourceConfig.h"
#include <unordered_map>
#include <unordered_set>

struct GBufferTargets
{
    CFrameBuffer framebuffer;
    std::shared_ptr<CTexture> depth, albedoGlow, normalSpecular;
};
