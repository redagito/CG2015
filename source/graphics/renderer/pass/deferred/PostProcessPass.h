#pragma once
#include "graphics/renderer/DeferredRenderTargets.h"
#include "graphics/renderer/core/CRenderProfiler.h"
class IResourceManager;
class IScene;
class ICamera;
class IWindow;
class IGraphicsResourceManager;
class ISceneQuery;
#include "PresentationPass.h"

class PostProcessPass
{
public:
    PostProcessPass(const PostProcessPass&) = delete;
    PostProcessPass& operator=(const PostProcessPass&) = delete;
    PostProcessPass(
        const GBufferTargets& gbuffer,
        PostProcessTargets& post,
        DeferredFrameState& frame,
        const SunState& sun,
        FullscreenResources& fullscreen,
        CRenderProfiler& profiler,
        PresentationPass& presentation)
        : m_gbuffer(gbuffer),
          m_post(post),
          m_frame(frame),
          m_sun(sun),
          m_fullscreen(fullscreen),
          m_profiler(profiler),
          m_presentation(presentation) {}
    void render(
        const ICamera& camera,
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& texture);
    void godRayPass1(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& texture);
    void lensFlarePass2(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& sceneTexture);
    bool init(IResourceManager& manager);
private:
    void fxaaPass(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& texture);
    void fogPass(
        const ICamera& camera,
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& texture);
    void gaussBlurVerticalPass(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& texture);
    void gaussBlurHorizontalPass(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& texture);
    void depthOfFieldPass(
        const ICamera& camera,
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& sceneTexture,
        const std::shared_ptr<CTexture>& blurTexture);
    void godRayPass2(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& sceneTexture,
        const std::shared_ptr<CTexture>& godrayTexture);
    void vignetteBlurPass(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& texture);
    void bloomPass1(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& texture);
    void bloomPass2(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& sceneTexture,
        const std::shared_ptr<CTexture>& bloomTexture);
    void lensFlarePass(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& sceneTexture);
    void lensFlarePass3(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& sceneTexture,
        const std::shared_ptr<CTexture>& lensTexture);
    void celPass(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& sceneTexture);
    void toneMapPass(
        const IWindow& window,
        const IGraphicsResourceManager& manager,
        const std::shared_ptr<CTexture>& texture);
    bool initDepthOfFieldPass(IResourceManager& manager);
    bool initGaussBlurVerticalPass(IResourceManager& manager);
    bool initGaussBlurHorizontalPass(IResourceManager& manager);
    bool initFxaaPass(IResourceManager& manager);
    bool initFogPass(IResourceManager& manager);
    bool initGodRayPass1(IResourceManager& manager);
    bool initGodRayPass2(IResourceManager& manager);
    bool initVignetteBlurPass(IResourceManager& manager);
    bool initBloomPass1(IResourceManager& manager);
    bool initBloomPass2(IResourceManager& manager);
    bool initLensFlarePass(IResourceManager& manager);
    bool initLensFlarePass2(IResourceManager& manager);
    bool initLensFlarePass3(IResourceManager& manager);
    bool initCelPass(IResourceManager& manager);
    bool initToneMapPass(IResourceManager& manager);
    const GBufferTargets& m_gbuffer;
    PostProcessTargets& m_post;
    DeferredFrameState& m_frame;
    const SunState& m_sun;
    FullscreenResources& m_fullscreen;
    CRenderProfiler& m_profiler;
    PresentationPass& m_presentation;
    ResourceId m_bloomPass1ShaderId = invalidResource;
    ResourceId m_bloomPass2ShaderId = invalidResource;
    ResourceId m_celPassShaderId = invalidResource;
    ResourceId m_depthOfFieldPassShaderId = invalidResource;
    ResourceId m_fogPassShaderId = invalidResource;
    ResourceId m_fxaaPassShaderId = invalidResource;
    ResourceId m_gaussBlurHorizontalShaderId = invalidResource;
    ResourceId m_gaussBlurVerticalShaderId = invalidResource;
    ResourceId m_godRayPass1ShaderId = invalidResource;
    ResourceId m_godRayPass2ShaderId = invalidResource;
    ResourceId m_lensFlarePass2ShaderId = invalidResource;
    ResourceId m_lensFlarePass3ShaderId = invalidResource;
    ResourceId m_lensFlarePassShaderId = invalidResource;
    ResourceId m_toneMapPassShaderId = invalidResource;
    ResourceId m_vignetteBlurPassShaderId = invalidResource;
};
