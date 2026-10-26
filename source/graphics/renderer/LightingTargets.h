#pragma once
#include "CFrameBuffer.h"
#include "graphics/SceneConfig.h"
#include "math/CTransformer.h"
#include "resource/ResourceConfig.h"
#include <unordered_map>
#include <unordered_set>

struct LightingTargets
{
    CFrameBuffer framebuffer;
    std::shared_ptr<CTexture> diffuse, specular;
};
