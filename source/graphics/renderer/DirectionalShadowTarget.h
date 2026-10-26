#pragma once
#include <glm/glm.hpp>
#include <memory>
class CTexture;

struct DirectionalShadowTarget
{
    std::shared_ptr<CTexture> depth;
    glm::mat4 projectionBias = glm::mat4(1.f);
    float bias = 0.0005f;
};
