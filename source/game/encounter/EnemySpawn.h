#pragma once
#include <cstddef>
#include <glm/glm.hpp>
struct EnemySpawn
{
    std::size_t type = 0, ordinal = 1;
    float time = 0, phase = 0;
    glm::vec3 offset{0}; // Fraction of arena half-extents.
    std::size_t wave = 0;
    float playerDistance = 0;
};
