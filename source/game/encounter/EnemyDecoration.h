#pragma once
#include <glm/glm.hpp>
#include <string>
struct EnemyDecoration
{
    std::string mesh, material;
    glm::vec3 scale{.5f};
};
