#pragma once
#include "graphics/renderer/DeferredRenderTargets.h"
class IResourceManager;
class IScene;
class ICamera;
class IWindow;
class IGraphicsResourceManager;
class ISceneQuery;

class CompositionPass
{
public:
    CompositionPass(const CompositionPass&) = delete;
    CompositionPass& operator=(const CompositionPass&) = delete;
    CompositionPass(
        const GBufferTargets& gbuffer,
        const LightingTargets& lighting,
        ColorTarget& sceneColor,
        FullscreenResources& fullscreen)
        : m_gbuffer(gbuffer),
          m_lighting(lighting),
          m_sceneColor(sceneColor),
          m_fullscreen(fullscreen) {}
    void render(
        const IScene& scene,
        const ICamera& camera,
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        ISceneQuery& query);
    bool init(IResourceManager& manager);
private:

    const GBufferTargets& m_gbuffer;
    const LightingTargets& m_lighting;
    ColorTarget& m_sceneColor;
    FullscreenResources& m_fullscreen;
    ResourceId m_illuminationPassShaderId = invalidResource;
};
