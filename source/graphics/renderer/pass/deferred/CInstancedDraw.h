#pragma once
#include "resource/ResourceConfig.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <map>
#include <tuple>
#include <vector>
class IGraphicsResourceManager;
class CShaderProgram;
// Opaque geometry and shadow passes group identical mesh/material/winding combinations.
class CInstancedDraw
{
  public:
    CInstancedDraw();
    ~CInstancedDraw();
    CInstancedDraw(const CInstancedDraw &) = delete;
    CInstancedDraw &operator=(const CInstancedDraw &) = delete;
    void begin();
    void add(ResourceId mesh, ResourceId material, const glm::mat4 &model, bool mirrored);
    void draw(const IGraphicsResourceManager &manager, CShaderProgram &shader);

  private:
    GLuint m_buffer = 0;
    std::map<std::tuple<ResourceId, ResourceId, bool>, std::vector<glm::mat4>> m_batches;
};
