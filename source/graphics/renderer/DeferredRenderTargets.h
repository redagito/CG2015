#pragma once
#include "CFrameBuffer.h"
#include "graphics/SceneConfig.h"
#include "resource/ResourceConfig.h"
#include "math/CTransformer.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>

// Named attachment contracts shared by the deferred passes. Formats and
// attachment locations are established by each producing pass during init.
struct GBufferTargets {
    CFrameBuffer framebuffer;
    std::shared_ptr<CTexture> depth, albedoGlow, normalSpecular;
};
struct LightingTargets {
    CFrameBuffer framebuffer;
    std::shared_ptr<CTexture> diffuse, specular;
};
struct ColorTarget {
    CFrameBuffer framebuffer;
    std::shared_ptr<CTexture> color;
};
struct PostProcessTargets {
    // Reused in the existing effect order; these are storage roles, not effects.
    ColorTarget primary, secondary, scratch;
    std::shared_ptr<CTexture> output;
};
struct ShadowTargets {
    struct Directional { std::shared_ptr<CTexture> depth; glm::mat4 projectionBias=glm::mat4(1.f); float bias=0.0005f; };
    struct Point { std::shared_ptr<CTexture> depth, cube; };
    CFrameBuffer directionalFramebuffer, pointFramebuffer;
    std::shared_ptr<CTexture> directionalDepth, pointDepth, pointCube;
    std::unordered_map<SceneObjectId,Directional> directional;
    std::unordered_map<SceneObjectId,Point> point;
    std::unordered_set<SceneObjectId> renderedDirectional, renderedPoint;
    void prune();
};
struct SunState {
    glm::vec2 screen=glm::vec2(0.5f);
    glm::vec3 color=glm::vec3(0.f);
    bool visible=false;
};
struct DeferredFrameState {
    CTransformer transform;
    bool hasBlendedObjects=false;
};
struct FullscreenResources { ResourceId quad=invalidResource; };
struct DeferredRenderTargets {
    GBufferTargets gbuffer;
    ShadowTargets shadows;
    LightingTargets lighting;
    ColorTarget sceneColor;
    PostProcessTargets post;
};
