#pragma once
#include "CFrameBuffer.h"
#include "graphics/SceneConfig.h"
#include "math/CTransformer.h"
#include "resource/ResourceConfig.h"
#include <unordered_map>
#include <unordered_set>

struct ColorTarget
{
    CFrameBuffer framebuffer;
    std::shared_ptr<CTexture> color;
};
