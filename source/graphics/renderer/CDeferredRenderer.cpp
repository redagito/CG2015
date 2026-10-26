#include "CDeferredRenderer.h"
#include "core/CScopedRenderState.h"
#include "graphics/IScene.h"
#include "graphics/scene/CSceneQuery.h"
#include "graphics/IWindow.h"
#include "debug/RendererDebug.h"
#include <spdlog/spdlog.h>

bool CDeferredRenderer::init(IResourceManager& manager)
{
    SPDLOG_INFO("Initializing deferred renderer.");
    if (!m_background.init(manager) || !m_transparency.init(manager)) return false;
    std::string error;
    if (hasGLError(error)) { SPDLOG_ERROR("GL Error: {}", error.c_str()); return false; }

    // Keep producer initialization and resource loading in the original order.
    return m_geometry.init(manager)
        && m_shadowPass.init(manager)
        && m_lightingPass.init(manager)
        && m_composition.init(manager)
        && m_postProcess.init(manager)
        && m_presentation.init(manager);
}

void CDeferredRenderer::draw(const IScene& scene, const ICamera& camera, const IWindow& window,
                             const IGraphicsResourceManager& manager)
{
    // Draw init
    window.setActive();
    m_profiler.collect();
    CRenderProfiler::Scope frame(m_profiler, "frame");
    CScopedRenderState state;
    glDisable(GL_SCISSOR_TEST); glDisable(GL_STENCIL_TEST); glDisable(GL_RASTERIZER_DISCARD); glDisable(GL_FRAMEBUFFER_SRGB); glDisable(GL_POLYGON_OFFSET_FILL); glDisable(GL_DEPTH_CLAMP);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE); glDepthMask(GL_TRUE); glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    for (GLuint unit=0;unit<7;++unit) glBindSampler(unit,0);

    // Query visible scene objects and lights
	CSceneQuery query;
    scene.getVisibleObjects(camera, query);

    // Geometry pass fills gbuffer
    { CRenderProfiler::Scope timer(m_profiler, "geometry"); m_geometry.render(scene, camera, window, manager, query); }

    // Light pass fills lbuffer
    { CRenderProfiler::Scope timer(m_profiler, "lighting"); m_lightingPass.render(scene, camera, window, manager, query); }

    // Illumination pass renders lit scene from lbuffer and gbuffer
    { CRenderProfiler::Scope timer(m_profiler, "illumination"); m_composition.render(scene, camera, window, manager, query); }

    { CRenderProfiler::Scope timer(m_profiler, "background"); m_background.render(scene,camera,window,manager); }
    { CRenderProfiler::Scope timer(m_profiler, "transparency"); m_transparency.render(scene,camera,window,manager); }

    m_targets.shadows.prune();
    // Post processing pass
    { CRenderProfiler::Scope timer(m_profiler, "post_processing"); m_postProcess.render(camera, window, manager, m_targets.sceneColor.color); }

    m_presentation.render(camera, window, manager, m_targets);

    // Post draw error check
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
    }
}

CDeferredRenderer* CDeferredRenderer::create(IResourceManager& manager)
{
    CDeferredRenderer* renderer = new CDeferredRenderer;
    if (!renderer->init(manager))
    {
        delete renderer;
        renderer = nullptr;
        SPDLOG_ERROR("Failed to create deferred renderer, initialization failed.");
    }
    return renderer;
}
