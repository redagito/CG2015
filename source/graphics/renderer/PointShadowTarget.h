#pragma once
#include <glm/glm.hpp>
#include <memory>
class CTexture;

struct PointShadowTarget
{
    std::shared_ptr<CTexture> depth, cube;
};
