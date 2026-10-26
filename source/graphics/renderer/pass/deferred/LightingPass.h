#pragma once
#include "graphics/renderer/DeferredRenderTargets.h"
#include "graphics/renderer/core/CRenderProfiler.h"
class IResourceManager;
class IScene;
class ICamera;
class IWindow;
class IGraphicsResourceManager;
class ISceneQuery;
#include "ShadowPass.h"

class LightingPass
{
public:
    LightingPass(const LightingPass&) = delete;
    LightingPass& operator=(const LightingPass&) = delete;
    bool init(IResourceManager& manager);
    LightingPass(
        const GBufferTargets& gbuffer,
        LightingTargets& lighting,
        ShadowTargets& shadows,
        DeferredFrameState& frame,
        CRenderProfiler& profiler,
        ShadowPass& shadowPass)
        : m_gbuffer(gbuffer),
          m_lighting(lighting),
          m_shadows(shadows),
          m_frame(frame),
          m_profiler(profiler),
          m_shadowPass(shadowPass) {}
    void render(
        const IScene& scene,
        const ICamera& camera,
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        ISceneQuery& query);
private:
    void pointLightPass(
        const IScene& scene,
        const ICamera& camera,
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        ISceneQuery& query);
    void directionalLightPass(
        const IScene& scene,
        const ICamera& camera,
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        ISceneQuery& query);
    bool initPointLightPass(IResourceManager& manager);
    bool initDirectionalLightPass(IResourceManager& manager);
    const GBufferTargets& m_gbuffer;
    LightingTargets& m_lighting;
    ShadowTargets& m_shadows;
    DeferredFrameState& m_frame;
    CRenderProfiler& m_profiler;
    ShadowPass& m_shadowPass;
    ResourceId m_directionalLightPassShaderId = invalidResource;
    ResourceId m_directionalLightScreenQuadId = invalidResource;
    ResourceId m_pointLightPassShaderId = invalidResource;
    ResourceId m_pointLightSphereId = invalidResource;
};
