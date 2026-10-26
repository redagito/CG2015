#pragma once
#include "CFrameBuffer.h"
#include "ColorTarget.h"
#include "graphics/SceneConfig.h"
#include "math/CTransformer.h"
#include "resource/ResourceConfig.h"
#include <unordered_map>
#include <unordered_set>

struct PostProcessTargets
{
    // Reused in the existing effect order; these are storage roles, not effects.
    ColorTarget primary, secondary, scratch;
    std::shared_ptr<CTexture> output;
};
