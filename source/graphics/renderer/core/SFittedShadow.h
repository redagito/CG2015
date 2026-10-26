#pragma once
#include <glm/glm.hpp>

struct SFittedShadow
{
    glm::mat4 view, projection;
    float bias;
};
