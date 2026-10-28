#pragma once
#include <glm/glm.hpp>
#include <vector>
struct WeaponTuning
{
    glm::vec3 color{.25f, .85f, 2};
    float projectileSpeed = 90, projectileLifetime = 4, damage = 50, interval = .3f;
    // Model-space gun tips, extended enough to clear the projectile's rear cap.
    std::vector<glm::vec3> muzzles{{3.2264f, .424f, 7.1f}, {-2.6727f, .424f, 7.1f}};
};
