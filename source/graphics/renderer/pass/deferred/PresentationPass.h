#pragma once
#include "graphics/renderer/DeferredRenderTargets.h"
#include "graphics/renderer/core/CRenderProfiler.h"
class IResourceManager;
class IScene;
class ICamera;
class IWindow;
class IGraphicsResourceManager;
class ISceneQuery;

class PresentationPass
{
public:
    PresentationPass(const PresentationPass&) = delete;
    PresentationPass& operator=(const PresentationPass&) = delete;
    bool init(IResourceManager& manager);
    void render(const ICamera&, const IWindow&, const IGraphicsResourceManager&, const DeferredRenderTargets&);
    PresentationPass(
        const GBufferTargets& gbuffer,
        FullscreenResources& fullscreen,
        CRenderProfiler& profiler)
        : m_gbuffer(gbuffer),
          m_fullscreen(fullscreen),
          m_profiler(profiler) {}
    void passthroughPass(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& texture,
        bool encodeOutput = false);
private:
    void displayPass(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& texture);
    void visualizeDepthPass(const ICamera& camera, const IWindow& window, const IGraphicsResourceManager& manager);
    bool initDisplayPass(IResourceManager& manager);
    bool initVisualizeDepthPass(IResourceManager& manager);
    const GBufferTargets& m_gbuffer;
    FullscreenResources& m_fullscreen;
    CRenderProfiler& m_profiler;
    ResourceId m_displayPassShaderId = invalidResource;

    ResourceId m_visualizeDepthPassShaderId = invalidResource;
};
