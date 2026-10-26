#include "graphics/resource/CMaterial.h"
#include "graphics/resource/CMesh.h"
#pragma once
#include "graphics/renderer/DeferredRenderTargets.h"
class IResourceManager;
class IScene;
class ICamera;
class IWindow;
class IGraphicsResourceManager;
class ISceneQuery;

class TransparencyPass
{
  public:
    TransparencyPass(const TransparencyPass &) = delete;
    TransparencyPass &operator=(const TransparencyPass &) = delete;
    TransparencyPass(ColorTarget &sceneColor, const ShadowTargets &shadows)
        : m_sceneColor(sceneColor), m_shadows(shadows)
    {
    }
    bool init(IResourceManager &manager);
    void render(const IScene &scene, const ICamera &camera, const IWindow &window,
                const IGraphicsResourceManager &manager);

  private:
    struct Item
    {
        CMesh *mesh;
        CMaterial *material;
        glm::vec3 position, scale;
        glm::quat rotation;
        float distance;
    };
    struct Point
    {
        SceneObjectId id;
        glm::vec3 position, color;
        float radius;
    };
    struct Directional
    {
        SceneObjectId id;
        glm::vec3 direction, color;
    };

    ColorTarget &m_sceneColor;
    const ShadowTargets &m_shadows;
    ResourceId m_transparencyShader = invalidResource;
};
