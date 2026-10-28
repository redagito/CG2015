#pragma once
#include <glm/glm.hpp>
struct BoundaryResponse
{
    glm::vec3 inward{0};
    float strength = 0, clearance = 0;
};
