#pragma once
#include "graphics/renderer/ColorTarget.h"
class IScene;
class ICamera;
class IWindow;
class IResourceManager;
class IGraphicsResourceManager;
class ParticlePass
{
  public:
    explicit ParticlePass(ColorTarget &target);
    ~ParticlePass();
    bool init(IResourceManager &resources);
    void render(const IScene &scene, const ICamera &camera, const IWindow &window,
                const IGraphicsResourceManager &resources);

  private:
    ColorTarget &m_target;
    ResourceId m_shader = invalidResource;
    unsigned m_vao = 0, m_buffer = 0;
    std::vector<float> m_vertices;
};
