#pragma once
#include "graphics/renderer/DeferredRenderTargets.h"
class IResourceManager;
class IScene;
class ICamera;
class IWindow;
class IGraphicsResourceManager;
class ISceneQuery;

class ShadowPass
{
  public:
    ShadowPass(const ShadowPass &) = delete;
    ShadowPass &operator=(const ShadowPass &) = delete;
    bool init(IResourceManager &manager);
    ShadowPass(ShadowTargets &shadows) : m_shadows(shadows)
    {
    }
    void renderDirectional(const IScene &scene, const ICamera &camera, const IWindow &window,
                           const IGraphicsResourceManager &manager);
    void renderPoint(const IScene &scene, const ICamera &camera, const IWindow &window,
                     const IGraphicsResourceManager &manager);

  private:
    struct CameraDirection
    {
        GLenum cubemapFace;
        glm::vec3 target;
        glm::vec3 up;
    };
    static const CameraDirection g_cameraDirections[6];
    bool initShadowMapPass(IResourceManager &manager);
    bool initShadowCubePass(IResourceManager &manager);
    ShadowTargets &m_shadows;
    ResourceId m_shadowCubePassShaderId = invalidResource;
    ResourceId m_shadowMapPassShaderId = invalidResource;
};
