#pragma once
#include <glm/vec3.hpp>
class CCollidable;
struct CollisionHit
{
    CCollidable *entity = nullptr;
    float fraction = 1;
    glm::vec3 normal{0};
};
