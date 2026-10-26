#pragma once
#include "CFrameBuffer.h"
#include "DirectionalShadowTarget.h"
#include "PointShadowTarget.h"
#include "graphics/SceneConfig.h"
#include "math/CTransformer.h"
#include "resource/ResourceConfig.h"
#include <unordered_map>
#include <unordered_set>

struct ShadowTargets
{

    CFrameBuffer directionalFramebuffer, pointFramebuffer;
    std::shared_ptr<CTexture> directionalDepth, pointDepth, pointCube;
    std::unordered_map<SceneObjectId, DirectionalShadowTarget> directional;
    std::unordered_map<SceneObjectId, PointShadowTarget> point;
    std::unordered_set<SceneObjectId> renderedDirectional, renderedPoint;
    void prune();
};
