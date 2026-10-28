#pragma once
#include <glm/glm.hpp>
struct InterceptSolution
{
    float time = 0;
    glm::vec3 aimPoint{0}, impactPoint{0};
};
