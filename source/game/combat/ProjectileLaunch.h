#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
struct ProjectileLaunch
{
    glm::vec3 position{0}, velocity{0};
    glm::quat orientation{1, 0, 0, 0};
};
