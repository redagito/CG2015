#pragma once
#include "CInstancedDraw.h"
#include "graphics/renderer/DeferredRenderTargets.h"
class IResourceManager;
class IScene;
class ICamera;
class IWindow;
class IGraphicsResourceManager;
class ISceneQuery;

class GeometryPass
{
  public:
    GeometryPass(const GeometryPass &) = delete;
    GeometryPass &operator=(const GeometryPass &) = delete;
    GeometryPass(GBufferTargets &gbuffer, DeferredFrameState &frame)
        : m_gbuffer(gbuffer), m_frame(frame)
    {
    }
    void render(const IScene &scene, const ICamera &camera, const IWindow &window,
                const IGraphicsResourceManager &manager, ISceneQuery &query);
    bool init(IResourceManager &manager);

  private:
    CInstancedDraw m_instances;
    GBufferTargets &m_gbuffer;
    DeferredFrameState &m_frame;
    ResourceId m_geometryPassShaderId = invalidResource;
};
