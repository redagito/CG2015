#pragma once
#include "DeferredRenderTargets.h"
#include "graphics/IRenderer.h"
#include "pass/deferred/BackgroundPass.h"
#include "pass/deferred/CompositionPass.h"
#include "pass/deferred/GeometryPass.h"
#include "pass/deferred/LightingPass.h"
#include "pass/deferred/ParticlePass.h"
#include "pass/deferred/PostProcessPass.h"
#include "pass/deferred/PresentationPass.h"
#include "pass/deferred/ShadowPass.h"
#include "pass/deferred/TransparencyPass.h"

// Coordinates the fixed deferred pipeline. Passes own shaders; named targets
// own framebuffer storage and expose the producer/consumer attachment contract.
class CDeferredRenderer : public IRenderer
{
  public:
    CDeferredRenderer() = default;
    ~CDeferredRenderer() = default;
    const CRenderProfiler &profiler() const
    {
        return m_profiler;
    }
    bool init(IResourceManager &manager);
    void draw(const IScene &scene, const ICamera &camera, const IWindow &window,
              const IGraphicsResourceManager &manager);
    static CDeferredRenderer *create(IResourceManager &manager);

  protected:
    // Narrow compatibility hooks for numerical pass regression tests.
    void geometryPass(const IScene &scene, const ICamera &camera, const IWindow &window,
                      const IGraphicsResourceManager &manager, ISceneQuery &query)
    {
        m_geometry.render(scene, camera, window, manager, query);
    }
    void lightPass(const IScene &scene, const ICamera &camera, const IWindow &window,
                   const IGraphicsResourceManager &manager, ISceneQuery &query)
    {
        m_lightingPass.render(scene, camera, window, manager, query);
    }
    void illuminationPass(const IScene &scene, const ICamera &camera, const IWindow &window,
                          const IGraphicsResourceManager &manager, ISceneQuery &query)
    {
        m_composition.render(scene, camera, window, manager, query);
    }
    void backgroundPass(const IScene &scene, const ICamera &camera, const IWindow &window,
                        const IGraphicsResourceManager &manager)
    {
        m_background.render(scene, camera, window, manager);
    }
    void transparencyPass(const IScene &scene, const ICamera &camera, const IWindow &window,
                          const IGraphicsResourceManager &manager)
    {
        m_transparency.render(scene, camera, window, manager);
    }
    void postProcessPass(const ICamera &camera, const IWindow &window,
                         const IGraphicsResourceManager &manager,
                         const std::shared_ptr<CTexture> &texture)
    {
        m_postProcess.render(camera, window, manager, texture);
    }
    void lensFlarePass2(const IWindow &window, const IGraphicsResourceManager &manager,
                        const std::shared_ptr<CTexture> &sceneTexture)
    {
        m_postProcess.lensFlarePass2(window, manager, sceneTexture);
    }
    void godRayPass1(const IWindow &window, const IGraphicsResourceManager &manager,
                     const std::shared_ptr<CTexture> &texture)
    {
        m_postProcess.godRayPass1(window, manager, texture);
    }

  private:
    DeferredRenderTargets m_targets;
    DeferredFrameState m_frame;
    SunState m_sun;
    FullscreenResources m_fullscreen;
    CRenderProfiler m_profiler;
    ShadowPass m_shadowPass{m_targets.shadows};
    PresentationPass m_presentation{m_targets.gbuffer, m_fullscreen, m_profiler};
    GeometryPass m_geometry{m_targets.gbuffer, m_frame};
    LightingPass m_lightingPass{m_targets.gbuffer, m_targets.lighting, m_targets.shadows,
                                m_frame,           m_profiler,         m_shadowPass};
    CompositionPass m_composition{m_targets.gbuffer, m_targets.lighting, m_targets.sceneColor,
                                  m_fullscreen};
    BackgroundPass m_background{m_targets.sceneColor, m_sun, m_fullscreen};
    TransparencyPass m_transparency{m_targets.sceneColor, m_targets.shadows};
    ParticlePass m_particles{m_targets.sceneColor};
    PostProcessPass m_postProcess{m_targets.gbuffer, m_targets.post, m_frame,       m_sun,
                                  m_fullscreen,      m_profiler,     m_presentation};
};
