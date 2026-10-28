#pragma once
#include <glm/glm.hpp>
// Shared firing/lead contract: speed is relative to the shooter, velocity is inherited.
struct WeaponAim
{
    glm::vec3 muzzle{0}, forward{0, 0, 1}, inheritedVelocity{0};
    float speed = 90, lifetime = 4;
};
