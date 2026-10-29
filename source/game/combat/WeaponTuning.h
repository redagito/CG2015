#pragma once
#include "ProjectileModifiers.h"
#include <glm/glm.hpp>
#include <vector>
struct WeaponTuning
{
    unsigned volleySize = 1;
    float criticalChance = 0, criticalMultiplier = 1.5f;
    float volleyDamageMultiplier = 1;
    ProjectileModifiers modifiers;
    glm::vec3 color{.25f, .85f, 2};
    float projectileSpeed = 90, projectileLifetime = 4, damage = 50, interval = .3f;
    // Model-space gun tips, extended enough to clear the projectile's rear cap.
    std::vector<glm::vec3> muzzles{{3.2264f, .424f, 7.1f}, {-2.6727f, .424f, 7.1f}};
};
